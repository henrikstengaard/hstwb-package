# Build Package
# -------------
#
# Author: Henrik Nørfjand Stengaard
# Date:   2017-05-10
#
# A PowerShell script to build package for HstWB Installer.

using assembly ICSharpCode.SharpZipLib.dll
using namespace ICSharpCode.SharpZipLib.Zip
using namespace ICSharpCode.SharpZipLib.Checksums


# create amiga zip file
function CreateAmigaZipFile($zipFile)
{
    $zipFileStream = New-Object System.IO.FileStream($zipFile, 'Create')
	$zipOutputStream = New-Object ZipOutputStream($zipFileStream)

	$crc = New-Object Crc32

	$amigaHostSystemId = [HostSystemID]::Amiga
	$method = [CompressionMethod]::Deflate

    # $fileStream = New-Object System.IO.FileStream('c:\Work\First Realize\hstwb-package\hstwb\C\acatune', 'Open', 'Read', 'Read')

	# $fileStream.Close()
	# $fileStream.Dispose()

	$bytes = [System.IO.File]::ReadAllBytes('c:\Work\First Realize\hstwb-package\hstwb\C\acatune')

	# update crc
	$crc.Reset()
	$crc.Update($bytes)

	# add zip entry
	$zipEntry = New-Object ZipEntry("test/acatune")
	# $zipEntry.HostSystem = $amigaHostSystemId
	$zipEntry.Size = $bytes.Count
	$zipEntry.Crc = $crc.Value
	#$zipEntry.Flags = 2048
	#$zipEntry.DateTime = DateTime.Now;
	#$zip.Add($zipEntry)

	#$zipEntry.GetType().GetProperties() | Where-Object {$_.Name -match 'versionMadeBy'}


	#$zipEntry.VersionMadeBy

	#Write-Host $zipEntry.HostSystem

	$zipOutputStream.PutNextEntry($zipEntry);
	$zipOutputStream.Write($bytes, 0, $bytes.Count);

	$zipOutputStream.Close()
	$zipOutputStream.Dispose()
	$zipFileStream.Close()
	$zipFileStream.Dispose()
}


function ReadLocalFileEntry($binaryReader)
{
	$version = $binaryReader.ReadInt16()
	$flags = $binaryReader.ReadInt16()
	$method = $binaryReader.ReadInt16()
	$fileModificationTime = $binaryReader.ReadInt16()
	$fileModificationDate = $binaryReader.ReadInt16()
	$crc32 = $binaryReader.ReadInt32()
	$compressedSize = $binaryReader.ReadInt32()
	$uncompressedSize = $binaryReader.ReadInt32()
	$fileNameLength = $binaryReader.ReadInt16()
	$extraFieldLength = $binaryReader.ReadInt16()

	$fileNameBytes = $binaryReader.ReadBytes($fileNameLength)
	$extraFieldBytes = $binaryReader.ReadBytes($extraFieldLength)

	$encoding = [system.Text.Encoding]::UTF8

	#$fileName = $encoding.GetString($fileNameBytes)

	#if ()
	#$extraField = $encoding.GetString($extraFieldLength)

	#Write-Host $extraFieldLength

	return New-Object PSObject -Property @{`
		'Version' = $version;
		'Flags' = $flags;
		'Method' = $method;
		'FileModificationTime' = $fileModificationTime;
		'FileModificationDate' = $fileModificationDate;
		'Crc32' = $crc32;
		'CompressedSize' = $compressedSize;
		'UncompressedSize' = $uncompressedSize;
		'FileNameBytes' = $fileNameBytes;
		'ExtraFieldBytes' = $extraFieldBytes
	}

	#Write-Host ""
	#Write-Host "signature = $signature"
	# Write-Host "version = $version"
	# Write-Host "flags = $flags"
	# Write-Host "method = $method"
	# Write-Host "fileModificationTime = $fileModificationTime"
	# Write-Host "fileModificationDate = $fileModificationDate"
	# Write-Host "crc32 = $crc32"
	# Write-Host "compressedSize = $compressedSize"
	# Write-Host "uncompressedSize = $uncompressedSize"
	# Write-Host "fileNameLength = $fileNameLength"
	# Write-Host "extraFieldLength = $extraFieldLength"
	#Write-Host "fileName = $fileName"
	# Write-Host "extraField = $extraField"

	# if ($compressedSize -gt 0)
	# {
	# 	$binaryReader.BaseStream.Seek($compressedSize, ([System.IO.SeekOrigin]::Current)) | Out-Null
	# }
}

function WriteLocalFileEntry($binaryWriter, $localFileEntry)
{
	$binaryWriter.Write([Int]$localFileHeaderSignature)
	$binaryWriter.Write([Int16]$localFileEntry.Version)
	$binaryWriter.Write([Int16]$localFileEntry.Flags)
	$binaryWriter.Write([Int16]$localFileEntry.Method)
	$binaryWriter.Write([Int16]$localFileEntry.FileModificationTime)
	$binaryWriter.Write([Int16]$localFileEntry.FileModificationDate)
	$binaryWriter.Write([Int]$localFileEntry.Crc32)
	$binaryWriter.Write([int]$localFileEntry.CompressedSize)
	$binaryWriter.Write([Int]$localFileEntry.UncompressedSize)
	$binaryWriter.Write([Int16]$localFileEntry.FileNameBytes.Count)
	$binaryWriter.Write([Int16]$localFileEntry.ExtraFieldBytes.Count)

	if ($localFileEntry.FileNameBytes.Count)
	{
		$binaryWriter.Write($localFileEntry.FileNameBytes)
	}

	if ($localFileEntry.ExtraFieldBytes.Count)
	{
		$binaryWriter.Write($localFileEntry.ExtraFieldBytes)
	}
}

function CopyBytes($binaryReader, $binaryWriter, $count)
{
	[byte[]]$buffer = new-object byte[] 4096
	$offset = 0
	do{
		if ($offset + $buffer.Count -gt $count)
		{
			$length = $count - $offset
		}
		else
		{
			$length = $buffer.Count
		}
		$result = $binaryReader.BaseStream.Read($buffer, 0, $length)
#		Write-Host "Read $length, Result $result"
		$binaryWriter.BaseStream.Write($buffer, 0, $result)
		$offset += $result
	} while($result -eq $length -and $offset -lt $count)
}

function ReadCentralDirectoryFileEntry($binaryReader, $binaryWriter)
{
	#Write-Host $binaryWriter.BaseStream.Position
	#Write-Host $binaryReader.BaseStream.Position
	#exit
	#$binaryWriter.BaseStream.Position = $binaryReader.BaseStream.Position
	#$binaryWriter.Write([Int16]20)
	#$binaryWriter.Write([Int16]20)
	#$binaryWriter.Write([Int16]2048)
	#$binaryWriter.Flush()
	# write 
	$versionMadeBy = $binaryReader.ReadInt16()
	$version = $binaryReader.ReadInt16()
	$bitFlag = $binaryReader.ReadInt16()
	$method = $binaryReader.ReadInt16()
	$fileModificationTime = $binaryReader.ReadInt16()
	$fileModificationDate = $binaryReader.ReadInt16()
	$crc32 = $binaryReader.ReadInt32()
	$compressedSize = $binaryReader.ReadInt32()
	$uncompressedSize = $binaryReader.ReadInt32()
	$fileNameLength = $binaryReader.ReadInt16()
	$extraFieldLength = $binaryReader.ReadInt16()
	$fileCommentLength = $binaryReader.ReadInt16()
	$diskNumber = $binaryReader.ReadInt16()
	$internalFileAttributes = $binaryReader.ReadInt16()
	$externalFileAttributes = $binaryReader.ReadInt32()
	$offset = $binaryReader.ReadInt32()

	$fileNameBytes = $binaryReader.ReadBytes($fileNameLength)
	$extraFieldBytes = $binaryReader.ReadBytes($extraFieldLength)
	$fileCommentBytes = $binaryReader.ReadBytes($fileCommentLength)

	$encoding = [system.Text.Encoding]::UTF8

	$fileName = $encoding.GetString($fileNameBytes)

	# amiga zip has: versionMadeBy = 20 and bitflag = 2048

	Write-Host "versionMadeBy = $versionMadeBy"
	Write-Host "version = $version"
	Write-Host "bitFlag = $bitFlag"
	# Write-Host "compressedSize = $compressedSize"
	# Write-Host "uncompressedSize = $uncompressedSize"
	# Write-Host "fileNameLength = $fileNameLength"
	# Write-Host "extraFieldLength = $extraFieldLength"
	# Write-Host "fileCommentLength = $fileCommentLength"
	# Write-Host "offset = $offset"
	Write-Host "fileName = $fileName"
}


function ReadEndCentralDirectoryFileEntry($binaryReader)
{
	$diskNumber = $binaryReader.ReadInt16()
	$diskCentralStart = $binaryReader.ReadInt16()
	$numberOfCentralsStored = $binaryReader.ReadInt16()
	$totalNumberOfCentralDirectories = $binaryReader.ReadInt16()
	$sizeOfCentralDirectory = $binaryReader.ReadInt32()
	$offsetCentralDirectoryStart = $binaryReader.ReadInt32()
	$commentLength = $binaryReader.ReadInt16()
	$commentBytes = $binaryReader.ReadBytes($commentLength)
}

	$localFileHeaderSignature = 0x04034b50
	$centralDirectoryFileHeaderSignature = 0x02014b50
	$endCentralDirectoryFileHeaderSignature = 0x06054b50

function PatchZip($zipFile, $patchZipFile)
{
	$openFileMode = [System.IO.FileMode]::Open
	$readAccess = [System.IO.FileAccess]::Read
	$readFileShare = [System.IO.FileShare]::Read
    $zipFileStream = New-Object System.IO.FileStream $zipFile, $openFileMode, $readAccess, $readFileShare
    $zipFileReader = New-Object System.IO.BinaryReader($zipFileStream)

	$createFileMode = [System.IO.FileMode]::Create
	$writeAccess = [System.IO.FileAccess]::Write
	$writeFileShare = [System.IO.FileShare]::Write
    $patchZipFileStream = New-Object System.IO.FileStream $patchZipFile, $createFileMode, $writeAccess, $writeFileShare
    $patchZipFileWriter = New-Object System.IO.BinaryWriter($patchZipFileStream)


	do
	{
		$signature = $zipFileReader.ReadInt32()

		if ($signature -eq $localFileHeaderSignature)
		{
			$localFileEntry = ReadLocalFileEntry $zipFileReader
			#$localFileEntry
			WriteLocalFileEntry $patchZipFileWriter $localFileEntry

			#break

			if ($localFileEntry.CompressedSize -gt 0)
			{
				CopyBytes $zipFileReader $patchZipFileWriter $localFileEntry.CompressedSize
				#$binaryReader.BaseStream.Seek($localFileEntry.CompressedSize, ([System.IO.SeekOrigin]::Current)) | Out-Null
			}
		}
		elseif ($signature -eq $centralDirectoryFileHeaderSignature)
		{
			break
			ReadCentralDirectoryFileEntry $binaryReader $binaryWriter
		}
		elseif ($signature -eq $endCentralDirectoryFileHeaderSignature)
		{
			break
			ReadEndCentralDirectoryFileEntry $binaryReader
		}
		else
		{
			break
			$signature
		}
	}
	while ($signature -eq $localFileHeaderSignature -or $signature -eq $centralDirectoryFileHeaderSignature)

	$patchZipFileWriter.Close()
	$patchZipFileWriter.Dispose()

	$patchZipFileStream.Close()
	$patchZipFileStream.Dispose()

	$zipFileReader.Close()
	$zipFileReader.Dispose()

	$zipFileStream.Close()
	$zipFileStream.Dispose()
}

function CheckZip($zipFile)
{
	$zip = New-Object ZipFile($zipFile)
	$zip.BeginUpdate()

	#$amigaHostSystemId = [HostSystemID]::Amiga

	foreach($zipEntry in $zip)
	{
		# use reflection to get private property 'versionMadeBy' and change it
		$versionMadeByField = $zipEntry.GetType().GetField("versionMadeBy", [Reflection.BindingFlags]::NonPublic -bor [Reflection.BindingFlags]::Instance)
		$versionMadeBy = [uint32]20
		$versionMadeByField.SetValue($zipEntry, [UInt16]20)
		
		$zipEntry
		# if ($zipEntry.IsDirectory -or $zipEntry.Flags -eq 2048)
		# {
	    #     continue
		# }
		#$zipEntry.HostSystem = $amigaHostSystemId
		#$zipEntry.Flags = 2048
	}

	# # create amiga zip file
	# $zip = [ZipFile]::create($amigaZipFile)



	# # add zip entry
	# $zipEntry = [ZipEntry]::new("some/file")
	# $zipEntry.HostSystem = $amigaHostSystemId
	# $zip.Add($zipEntry)

	# # close amiga zip file
    $zip.CommitUpdate()
    $zip.Close()
}


#CreateAmigaZipFile $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath("test2.zip")

#Copy-Item 'c:\Work\First Realize\hstwb-package\package\hstwb_powershell.zip' -Destination 'c:\Work\First Realize\hstwb-package\package\hstwb_patched2.zip'   

PatchZip 'c:\Work\First Realize\hstwb-package\package\hstwb_powershell.zip' 'c:\Work\First Realize\hstwb-package\package\hstwb_patched3.zip'
#PatchZip $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath("test2.zip")

exit


Add-Type -Assembly System.IO.Compression.FileSystem


# paths
$compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
$rootDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath("..")
$packageDir = Join-Path -Path $rootDir -ChildPath "package"
$screenshotsDir = Join-Path -Path $rootDir -ChildPath "screenshots"
$readmeMarkdownFile = Join-Path -Path $rootDir -ChildPath "README.md"

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


# c# code for forward slash encoder used to create zip files with forward slash as path separator in entries compatible with amiga
$source = @"
using System.Text;

namespace HstWB.Package
{ 
	public class ForwardSlashEncoder : UTF8Encoding
	{
		public ForwardSlashEncoder() : base(true)
		{
		}

		public override byte[] GetBytes(string s)
		{
			s = s.Replace("\\", "/");
			return base.GetBytes(s);
		}
	}
}
"@
Add-Type -TypeDefinition $source -Language CSharp 


# create instance of forward slash encoder
$forwardSlashEncoder = New-Object 'HstWB.Package.ForwardSlashEncoder'


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

	# content zip file
	$contentZipFile = [System.IO.Path]::Combine($packageDir, ($contentDir.Name + ".zip"))

	# delete content zip file, if it exists
	if (test-path -path $contentZipFile)
	{
		remove-item $contentZipFile -force
	}

	# compress content directory
	[System.IO.Compression.ZipFile]::CreateFromDirectory($contentDir.FullName, $contentZipFile, $compressionLevel, $false, $forwardSlashEncoder)	
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
[System.IO.Compression.ZipFile]::CreateFromDirectory($packageDir, $packageFile, $compressionLevel, $false, $forwardSlashEncoder)


# write progress message
Write-Host "Done." -ForegroundColor "Yellow"
