$f = "C:\Users\Shuaizx\Desktop\23h\tmp\sch_extract\ProDoc_Schematic1_2026-09-11_copy_2026-09-12-21-06\ProDoc_Schematic1_2026-09-11_copy.epru"
$t = [System.IO.File]::ReadAllText($f)
$docs = $t -split '\|(?=\{)'
$n = 0
foreach ($d in $docs) {
    if ($d -match '"type":"PART"') {
        Write-Output "==== PART $n ===="
        if ($d.Length -gt 1200) { Write-Output $d.Substring(0,1200) } else { Write-Output $d }
        $n++
    }
}
Write-Output "PART count: $n"
