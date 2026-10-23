#!/usr/bin/env python3
"""Copies the apex-sdk-core sources into the plugin, where the ApexCore module compiles them.

    python3 Tools/update_core.py --source ../apex-sdk-core     from a local checkout
    python3 Tools/update_core.py --zip apex-sdk-core-v0.2.0.zip  from a release zip on disk

Writes Source/ThirdParty/ApexCore/:
    include/apex/Apex.h          the C interface
    src/                         the core's C++ sources, with the generated GraphqlQueries.hpp
    third_party/nlohmann/        nlohmann/json, the core's only dependency
    THIRD_PARTY_NOTICES.md
    VERSION.txt                  the core version, and the commit when copied from a checkout

The plugin never edits these files; update them only through this script.
"""
import argparse
import io
import os
import re
import shutil
import subprocess
import sys
import zipfile

PLUGIN_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TARGET_DIRECTORY = os.path.join(PLUGIN_ROOT, "Source", "ThirdParty", "ApexCore")
SOURCE_SUFFIXES = (".cpp", ".hpp", ".h", ".map")
COPIED_DIRECTORIES = ("include/", "src/", "third_party/nlohmann/")
COPIED_FILES = ("THIRD_PARTY_NOTICES.md",)


def ReadCheckout(checkoutDirectory):
	"""Returns {relative path: bytes} for the files the plugin needs from a checkout."""
	files = {}
	for copiedDirectory in COPIED_DIRECTORIES:
		absoluteDirectory = os.path.join(checkoutDirectory, copiedDirectory)
		for directoryPath, _, fileNames in os.walk(absoluteDirectory):
			for fileName in fileNames:
				absolutePath = os.path.join(directoryPath, fileName)
				relativePath = os.path.relpath(absolutePath, checkoutDirectory).replace(os.sep, "/")
				if fileName.endswith(SOURCE_SUFFIXES) or relativePath.startswith("third_party/"):
					with open(absolutePath, "rb") as sourceFile:
						files[relativePath] = sourceFile.read()

	for copiedFile in COPIED_FILES:
		with open(os.path.join(checkoutDirectory, copiedFile), "rb") as sourceFile:
			files[copiedFile] = sourceFile.read()

	files["src/GraphqlQueries.hpp"] = GenerateGraphqlQueries(checkoutDirectory)
	return files


def GenerateGraphqlQueries(checkoutDirectory):
	"""Does what the core's CMake configure step does with src/GraphqlQueries.hpp.in."""
	with open(os.path.join(checkoutDirectory, "src", "GraphqlQueries.hpp.in"), encoding="utf-8") as templateFile:
		headerText = templateFile.read()

	replacements = {
		"@APEX_GQL_GENERATE_AUTH_CODE@": "spec/graphql/generate_auth_code.graphql",
		"@APEX_GQL_USER_MODULES@": "spec/graphql/user_modules.graphql",
	}
	for placeholder, documentPath in replacements.items():
		with open(os.path.join(checkoutDirectory, documentPath), encoding="utf-8") as documentFile:
			headerText = headerText.replace(placeholder, documentFile.read().strip())

	return headerText.encode("utf-8")


def ReadCoreVersion(checkoutDirectory):
	with open(os.path.join(checkoutDirectory, "CMakeLists.txt"), encoding="utf-8") as cmakeFile:
		versionMatch = re.search(r"project\(apex_sdk_core VERSION ([0-9.]+)", cmakeFile.read())
	return versionMatch.group(1) if versionMatch else "unknown"


def ReadCommit(checkoutDirectory):
	try:
		return subprocess.check_output(["git", "-C", checkoutDirectory, "rev-parse", "HEAD"], text=True).strip()
	except (OSError, subprocess.CalledProcessError):
		return "unknown"


def ReadZip(zipPath):
	"""Returns {relative path: bytes} from a release zip, whose src/ already holds GraphqlQueries.hpp."""
	files = {}
	with zipfile.ZipFile(zipPath) as releaseZip:
		entryNames = releaseZip.namelist()
		# A release zip may put everything under one top-level folder.
		hasTopFolder = not any(entryName.startswith("include/") for entryName in entryNames)
		for entryName in entryNames:
			if entryName.endswith("/"):
				continue

			relativePath = entryName.split("/", 1)[-1] if hasTopFolder else entryName

			if relativePath.startswith(COPIED_DIRECTORIES) or relativePath in COPIED_FILES:
				files[relativePath] = releaseZip.read(entryName)

	if "src/GraphqlQueries.hpp" not in files:
		sys.exit("The zip has no src/GraphqlQueries.hpp; is it an apex-sdk-core release zip?")

	return files


def WriteFiles(files, versionText):
	if os.path.isdir(TARGET_DIRECTORY):
		shutil.rmtree(TARGET_DIRECTORY)

	for relativePath, fileBytes in sorted(files.items()):
		if relativePath.endswith(".hpp.in"):
			continue

		targetPath = os.path.join(TARGET_DIRECTORY, relativePath)
		os.makedirs(os.path.dirname(targetPath), exist_ok=True)
		with open(targetPath, "wb") as targetFile:
			targetFile.write(fileBytes)

	with open(os.path.join(TARGET_DIRECTORY, "VERSION.txt"), "w", encoding="utf-8", newline="\n") as versionFile:
		versionFile.write(versionText)


def Main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	sourceGroup = parser.add_mutually_exclusive_group(required=True)
	sourceGroup.add_argument("--source", help="a local apex-sdk-core checkout")
	sourceGroup.add_argument("--zip", help="an apex-sdk-core release zip")
	arguments = parser.parse_args()

	if arguments.source:
		checkoutDirectory = os.path.abspath(arguments.source)
		files = ReadCheckout(checkoutDirectory)
		versionText = "apex-sdk-core {0}\ncommit {1}\n".format(
			ReadCoreVersion(checkoutDirectory), ReadCommit(checkoutDirectory))
	else:
		files = ReadZip(arguments.zip)
		versionText = "apex-sdk-core {0}\n".format(os.path.basename(arguments.zip))

	WriteFiles(files, versionText)
	print("Copied {0} files into {1}".format(len(files), os.path.relpath(TARGET_DIRECTORY, PLUGIN_ROOT)))
	print(versionText.strip())


if __name__ == "__main__":
	Main()
