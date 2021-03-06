#NOTE: a requirement of this script is setting the ExecutionPolicy to RemoteSigned.
#      To do this (required only once), run PowerShell as Admin (search PowerShell
#      from start, right click & select run as admin) and run the command:
#      Set-ExecutionPolicy RemoteSigned

#re above, not req'd now due to ews dll???

############## Variables to Edit ################

#The hostname of your Exchange host
$server = "mail.yourserver.net"

#Your email address (should be your UPN -- i.e. "primary" address)
$email_address = "you@yourserver.net"

$GraphSize = 8  #Number of RGB elements in graph

$OnesColour = 0x00FF00      #green
$TensColour = 0xFFFF00      #yellow
$HundredsColour = 0xFF0000  #red

$port = "auto" #specify port or "auto" to search

############## Edit below at own risk ###############

############## Functions ###############

function testPort {
    param( [String]$port );

    #rather than -erroraction stop all the subsequent commands, lets just do this so our errors get caught
    $ErrorActionPreference = Stop
    try {
        $Serial = new-Object System.IO.Ports.SerialPort $port,115200,None,8,one
        $Serial.open();
        #need to give some startup time
        Start-Sleep -s 1
        #note: testcmd is actually an 'echo' function of the Arduino firmware I wrote
        $Serial.WriteLine("testcmd The Quick Brown Fox.");
        Start-Sleep -s 1
        #Future feature: verify the output. For now if we didn't error out, the port is ok
        return true;
    } catch {
        Write-debug "Testing port $port threw error $(_.Exception.Message)";
        return false;
    } finally {
        $Serial.Close()
    }
}


############## Main Code ###############

if($port -eq "auto") {
    $ports=[System.IO.Ports.SerialPort]::getportnames();
    $ports=$ports.Split('`n');
    $found = $False;
    foreach($p in $ports) {
        if(testPort -port $p) {
            $port = $p;
            $found = $True;
        }
    }
    if(-not $found) {
        Write-Error -ErrorAction Stop "Auto detection of COM port failed. Did you install the USB->COM drivers?"
        #nb: above -ea stop causes script execution to halt
    }
}

[void][Reflection.Assembly]::LoadFile(".\Microsoft.Exchange.WebServices.dll")

## Choose to ignore any SSL Warning issues caused by Self Signed Certificates  
  
## Code From http://poshcode.org/624
## Create a compilation environment
$Provider=New-Object Microsoft.CSharp.CSharpCodeProvider
$Compiler=$Provider.CreateCompiler()
$Params=New-Object System.CodeDom.Compiler.CompilerParameters
$Params.GenerateExecutable=$False
$Params.GenerateInMemory=$True
$Params.IncludeDebugInformation=$False
$Params.ReferencedAssemblies.Add("System.DLL") | Out-Null

$TASource=@'
  namespace Local.ToolkitExtensions.Net.CertificatePolicy{
    public class TrustAll : System.Net.ICertificatePolicy {
      public TrustAll() {
      }
      public bool CheckValidationResult(System.Net.ServicePoint sp,
        System.Security.Cryptography.X509Certificates.X509Certificate cert,
        System.Net.WebRequest req, int problem) {
        return true;
      }
    }
  }
'@
$TAResults=$Provider.CompileAssemblyFromSource($Params,$TASource)
$TAAssembly=$TAResults.CompiledAssembly

## We now create an instance of the TrustAll and attach it to the ServicePointManager
$TrustAll=$TAAssembly.CreateInstance("Local.ToolkitExtensions.Net.CertificatePolicy.TrustAll")
[System.Net.ServicePointManager]::CertificatePolicy=$TrustAll

## end code from http://poshcode.org/624

$Service = New-Object Microsoft.Exchange.WebServices.Data.ExchangeService
$Service.UseDefaultCredentials = $true
 
#  Specify an SMTP address.  The autodiscover URL from the associated mailbox will be used to connect to Exchange
#  Email specified here is used to get which server to query
$Service.AutodiscoverUrl($email_address)
#$service.Url = "https://$server/EWS/Exchange.asmx"

$inbox = [Microsoft.Exchange.WebServices.Data.Folder]::Bind($service, [Microsoft.Exchange.WebServices.Data.WellKnownFolderName]::Inbox)
"Number or Unread Messages : " + $inbox.UnreadCount

Switch($inbox.UnreadCount) {
    {$_ -lt 10} {
        $divisor=10;
        $colour = $OnesColour;
    }
    {{$_ -gt 10} -and {$_ -lt 100}} {
        $divisor=100;
        $colour = $TensColour;
    }
    {$_ -gt 100} {
        $divisor=1000;
        $colour = $HundredsColour;
    }
}

$factor=(($inbox.UnreadCount/$divisor)*$GraphSize);
$full = [math]::truncate([decimal]$factor);
$partial = $factor - $full;

Write-Debug ("LEDs 0-" + ($full-1) + " colour 0x" +("{0:X0}" -f $colour));

$r=$colour -shr 16;
$g=($colour -shr 8) - ($r -shl 8);
$b=$colour - ($r -shl 16) - ($g -shl 8);

Write-Verbose ("r = 0x" + ([Convert]::ToString($r,16)))

Write-Verbose ("Red = 0x" +("{0:X0}" -f $r) +", reduced by "+($partial*100)+"% now 0x" +("{0:X0}" -f [String]($r=[math]::Round($r * $partial))));
Write-Verbose ("Green = 0x" +("{0:X0}" -f $g) +", reduced by "+($partial*100)+"% now 0x" +("{0:X0}" -f [String]($g=[math]::Round($g * $partial))));
Write-Verbose ("Blue = 0x" +("{0:X0}" -f $b) +", reduced by "+($partial*100)+"% now 0x" +("{0:X0}" -f [String]($b=[math]::Round($b * $partial))));

Write-Verbose ("r = 0x" + ([Convert]::ToString($r,16)))
$r=$r -shl 16;
Write-Verbose ("r = 0x" + ([Convert]::ToString($r,16)))


Write-Verbose ("r = 0x" + ("{0:X0}" -f ($r -shl 16)) +", g = 0x"+("{0:X0}" -f ($g -shl 8))+ ", b= 0x"+("{0:X0}" -f [String]($b)))

[String]$partialColour=[int]($r -shl 16)+[int]($g -shl 8)+$b;

Write-Debug ("LED $full reduced by "+($partial*100)+"% to colour 0x" +("{0:X0}" -f $partialColour));

$Serial = new-Object System.IO.Ports.SerialPort $port,115200,None,8,one
$Serial.open();
#need to give some startup time
Start-Sleep -s 1
$Serial.WriteLine("set-lights 0-" + ($GraphSize-1) + " 0");
Start-Sleep -Milliseconds 1;
$Serial.WriteLine("set-lights 0-" + ($full-1) + " "+("{0:X0}" -f $colour));
Start-Sleep -Milliseconds 1;
$Serial.WriteLine("set-lights " + $full + " "+("{0:X0}" -f $partialColour));
#Start-Sleep -Milliseconds 250;
$Serial.Close()


