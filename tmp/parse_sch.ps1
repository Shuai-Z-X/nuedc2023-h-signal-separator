$f = "C:\Users\Shuaizx\Desktop\23h\tmp\sch_extract\ProDoc_Schematic1_2026-09-11_copy_2026-09-12-21-06\ProDoc_Schematic1_2026-09-11_copy.epru"
$t = [System.IO.File]::ReadAllText($f)
$docs = $t -split '\|(?=\{)'
$rows = @()
foreach ($d in $docs) {
    if ($d -match '"key":"Value"' -and $d -match '"x":-?[\d.]+') {
        $xv = [regex]::Match($d, '"x":(-?[\d.]+)').Groups[1].Value
        $yv = [regex]::Match($d, '"y":(-?[\d.]+)').Groups[1].Value
        $x = [double]$xv; $y = [double]$yv
        $val = [regex]::Match($d, '"value":"([^"]{0,20})"').Groups[1].Value
        if (($x -gt 550) -and ($x -lt 2300) -and ($y -gt -750) -and ($y -lt -250)) {
            $rows += [pscustomobject]@{ X = $x; Y = $y; V = $val }
        }
    }
}
($rows | Sort-Object X, Y | Format-Table -AutoSize | Out-String -Width 200) | Set-Content -Encoding UTF8 "C:\Users\Shuaizx\Desktop\23h\tmp\out_values.txt"
Write-Output "rows: $($rows.Count)"
