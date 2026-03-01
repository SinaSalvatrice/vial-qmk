param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$QmkArgs
)

$bash = "C:\QMK_MSYS\usr\bin\bash.exe"
if (-not (Test-Path $bash)) {
    Write-Error "QMK MSYS not found at $bash"
    exit 1
}

$joined = ($QmkArgs | ForEach-Object { $_.Replace('"', '\"') }) -join ' '
$cmd = "export MSYSTEM=MINGW64; export PATH=/mingw64/bin:/usr/bin; export SHELL=/usr/bin/bash; cd /c/vial-qmk; /mingw64/bin/qmk $joined"

& $bash --noprofile --norc -lc $cmd
exit $LASTEXITCODE
