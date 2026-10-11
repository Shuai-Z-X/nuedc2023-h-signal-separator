$f = "C:\Users\Shuaizx\Desktop\23h\tmp\sch_extract\ProDoc_Schematic1_2026-09-11_copy_2026-09-12-21-06\ProDoc_Schematic1_2026-09-11_copy.epru"
$t = [System.IO.File]::ReadAllText($f)
$docs = $t -split '\|(?=\{)'
$sb = New-Object System.Text.StringBuilder
$n = 0
foreach ($d in $docs) {
    if ($d -match '"type":"PART"') {
        [void]$sb.AppendLine("==== PART $n ====")
        if ($d.Length -gt 1500) { [void]$sb.AppendLine($d.Substring(0,1500)) } else { [void]$sb.AppendLine($d) }
        $n++
    }
}
[void]$sb.AppendLine("PART count: $n")
Set-Content -Encoding UTF8 "C:\Users\Shuaizx\Desktop\23h\tmp\part_docs.txt" $sb.ToString()
Write-Output "done $n"
