$f = "C:\Users\Shuaizx\Desktop\23h\tmp\sch_extract\ProDoc_Schematic1_2026-09-11_copy_2026-09-12-21-06\ProDoc_Schematic1_2026-09-11_copy.epru"
$t = [System.IO.File]::ReadAllText($f)
$docs = $t -split '\|(?=\{)'
$rows = @()
foreach ($d in $docs) {
    if ($d -match '"type":"COMPONENT"' -and $d -match '"x":-?[\d.]+') {
        $xv = [regex]::Match($d, '"x":(-?[\d.]+)').Groups[1].Value
        $yv = [regex]::Match($d, '"y":(-?[\d.]+)').Groups[1].Value
        $x = [double]$xv; $y = [double]$yv
        if (($x -gt 550) -and ($x -lt 2300) -and ($y -gt -750) -and ($y -lt -250)) {
            $des = [regex]::Match($d, '"designator":"([^"]{0,12})"').Groups[1].Value
            $name = [regex]::Match($d, '"name":"([^"]{0,30})"').Groups[1].Value
            $rows += [pscustomobject]@{ X = $x; Y = $y; Des = $des; Name = $name }
        }
    }
}
$out = $rows | Sort-Object X, Y | Format-Table -AutoSize | Out-String -Width 200
Set-Content -Encoding UTF8 "C:\Users\Shuaizx\Desktop\23h\tmp\out_components.txt" $out
Write-Output "rows: $($rows.Count)"
Write-Output $out
