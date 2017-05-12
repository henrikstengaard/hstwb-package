# Build Package
# -------------
#
# Author: Henrik Nørfjand Stengaard
# Date:   2017-05-13
#
# A PowerShell script to build package for HstWB Installer.


Add-Type -Assembly System.IO.Compression.FileSystem


# create zip from directory
function CreateZipFromDirectory($directoryName, $outputZipFile)
{
    # create zip archive
    $zipFileStream = New-Object System.IO.FileStream $outputZipFile, 'Create', 'Write', 'Write'
    $zipArchive = New-Object System.IO.Compression.ZipArchive $zipFileStream, 'Create'

	$items = @()
	$items += Get-ChildItem $directoryName -Recurse

	foreach($item in $items)
	{
		$entryName = $item.FullName.Substring($directoryName.Length + 1) -replace '\\', '/'

		# add tailing slash to entry name, if item is a directory
		if ($item.PSIsContainer)
		{
			$entryName += '/'
		}

		$zipArchiveEntry = $zipArchive.CreateEntry($entryName)

		# add item content to zip archive entry, if item is a file
		if (!$item.PSIsContainer)
		{
			# open entry stream and item file stream
			$entryStream = $zipArchiveEntry.Open()
			$itemFileStream = New-Object System.IO.FileStream $item.FullName, 'Open', 'Read'

			[byte[]]$buffer = new-object byte[] 4096

			# copy item file stream to entry stream
			do
			{
				$result = $itemFileStream.Read($buffer, 0, $buffer.Count)
				$entryStream.Write($buffer, 0, $result)
			} while ($result -eq $buffer.Count)

			# close and dispose zip archive and zip file stream
			$itemFileStream.Close()
			$itemFileStream.Dispose()
			$entryStream.Close()
		}
	}

	# close and dispose zip archive and zip file stream
    $zipArchive.Dispose()
	$zipFileStream.Close()
	$zipFileStream.Dispose()
}


# paths
$compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
$rootDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath("..")
$packageDir = Join-Path -Path $rootDir -ChildPath "package"
$screenshotsDir = Join-Path -Path $rootDir -ChildPath "screenshots"
$readmeMarkdownFile = Join-Path -Path $rootDir -ChildPath "README.md"
$convertZipToAmigaFile = Resolve-Path "convert_zip_to_amiga.ps1"


# get screenshot files
$screenshotFiles = @()

if (Test-Path $screenshotsDir)
{
	$screenshotFiles += Get-ChildItem -Path $screenshotsDir -Filter *.png
}


# copy screenshot files, if any exist
if ($screenshotFiles.Count -gt 0)
{
	Write-Host "Copying screenshots for package..." -ForegroundColor "Yellow"
	$packageScreenshotsDir = Join-Path -Path $packageDir -ChildPath "screenshots"

	if (!(Test-Path $packageScreenshotsDir))
	{
		mkdir $packageScreenshotsDir | Out-Null
	}

	$screenshotFiles | ForEach-Object { Copy-Item -Path $_.FullName -Destination $packageScreenshotsDir -Force }
	Write-Host "Done." -ForegroundColor "Yellow"
}


# Build html
Write-Host "Building readme html for package from readme markdown..." -ForegroundColor "Yellow"
& ".\build_html.ps1" -markdownFile $readmeMarkdownFile -htmlFile (Join-Path -Path $packageDir -ChildPath "README.html")
Write-Host "Done." -ForegroundColor "Yellow"


# Build guide
Write-Host "Building readme guide for package from readme markdown..." -ForegroundColor "Yellow"
& ".\build_guide.ps1" -markdownFile $readmeMarkdownFile -guideFile (Join-Path -Path $packageDir -ChildPath "README.guide")
Write-Host "Done." -ForegroundColor "Yellow"


# read package ini lines
$packageIniFile = [System.IO.Path]::Combine($packageDir, 'package.ini')
$packageIniLines = Get-Content $packageIniFile


# get package name and version from package ini lines
$packageName = ($packageIniLines | Where-Object { $_ -match '^Name' } | Select-Object -First 1) -replace '^Name=', ''
$packageVersion = ($packageIniLines | Where-Object { $_ -match '^Version' } | Select-Object -First 1) -replace '^Version=', ''


# write progress message
Write-Host "Build package '$packageName' v$packageVersion" -ForegroundColor "Yellow"


# compress content directories to zip files
$contentDirs = Get-ChildItem -Path $rootDir | Where-Object { $_.PSIsContainer -and $_.Name -notmatch '(package|screenshots|tools|licenses)' }
foreach ($contentDir in $contentDirs)
{
	# write progress message
	Write-Host ("Compressing content '" + $contentDir.Name + "' zip file...")

	# temp file file
	$tempZipFile = Join-Path $packageDir -ChildPath "temp.zip"

	# compress content directory
	CreateZipFromDirectory $contentDir.FullName $tempZipFile

	# content zip file
	$contentZipFile = Join-Path $packageDir -ChildPath ($contentDir.Name + ".zip")

	# delete content zip file, if it exists
	if (test-path -path $contentZipFile)
	{
		remove-item $contentZipFile -force
	}

	# convert zip to amiga
	& $convertZipToAmigaFile -zipFile $tempZipFile -outputZipFile $contentZipFile

	# delete temp zip file, if it exists
	if (test-path -path $tempZipFile)
	{
		remove-item $tempZipFile -force
	}
}


# write progress message
Write-Host "Compressing package zip file..." -ForegroundColor "Yellow"

# package file
$packageFile = Join-Path -Path $rootDir -ChildPath ("{0}.{1}.zip" -f ($packageName -replace '\s', '.'), $packageVersion)

# delete package file, if it exists
if (test-path -path $packageFile)
{
	remove-item $packageFile -force
}

# compress package directory
[System.IO.Compression.ZipFile]::CreateFromDirectory($packageDir, $packageFile, $compressionLevel, $false)


# write progress message
Write-Host "Done." -ForegroundColor "Yellow"