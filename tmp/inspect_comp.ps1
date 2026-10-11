$f = "C:\Users\Shuaizx\Desktop\23h\tmp\sch_extract\ProDoc_Schematic1_2026-09-11_copy_2026-09-12-21-06\ProDoc_Schematic1_2026-09-11_copy.epru"
$t = [System.IO.File]::ReadAllText($f)
$docs = $t -split '\|(?=\{)'
$n = 0
foreach ($d in $docs) {
    if ($d -match '"type":"COMPONENT"' -and $d -match 'NE5532') {
        Write-Output "==== COMPONENT with NE5532 ===="
        Write-Output $d
        $n++
        if ($n -ge 2) { break }
    }
}
