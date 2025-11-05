# Unreal Engine Plugin Makefile (Windows / PowerShell)
#
# Assumes Makefile is put next to .uplugin file, and make run at the root of plugin directory
#
# Tweak UE_ROOT and PLUGIN_NAME to your setup.
#
# Some requirements:
#
# - Powershell
# - jq (https://github.com/jqlang/jq) - choco install jq
# - nodejs (https://nodejs.org)

# Variables
# ---------
#
# Nice read on Makefile variables: https://earthly.dev/blog/makefile-variables/

# Current working directory - Works in Windows cmd or Make
CWD := $(shell echo %CD%)
HOME := $(shell echo %USERPROFILE%)

WORKDIR = .work
WORKDIR_ABSOLUTE = $(CWD)/$(WORKDIR)

# Change to your Unreal Engine root path
# UE_ROOT := C:/Path/To/UnrealEngine
UE_VERSION := 5.7
UE_ROOT := E:/Engines/UE_$(UE_VERSION)

UAT = $(UE_ROOT)/Engine/Build/BatchFiles/RunUAT.bat
UBT = $(UE_ROOT)/Engine/Build/BatchFiles/Build.bat
UE_EDITOR = $(UE_ROOT)/Engine/Binaries/Win64/UnrealEditor.exe
UE_CMD = $(UE_ROOT)/Engine/Binaries/Win64/UnrealEditor-Cmd.exe

# Change to your plugin name
PLUGIN_NAME := TargetSystem

UPLUGIN_NAME := $(shell jq -r '.FriendlyName' $(PLUGIN_NAME).uplugin)
UPLUGIN_VERSION := $(shell jq -r '.VersionName' $(PLUGIN_NAME).uplugin)

# Change to correct path to plugin root dir
PLUGIN_DIR := $(CWD)

# Example: ghcr.io/epicgames/unreal-engine:dev-slim-5.5
# See https://github.com/orgs/epicgames/packages/container/package/unreal-engine
DOCKER_UE_VERSION ?= 5.6
DOCKER_REGISTRY = ghcr.io/epicgames/unreal-engine
DOCKER_IMG = $(DOCKER_REGISTRY):dev-slim-$(DOCKER_UE_VERSION)

# Targets
# -------

all: help


# ## make uat
#
# UnrealAutomationTool (RunUAT.bat)
#
# Tweak UAT flags with `make uat UAT_FLAGS="BuildPlugin -Help"`
#
# make uat defaults to -Help, unless overriden via UAT_FLAGS="..." on the command line

uat: UAT_FLAGS ?= -Help
uat: 
	"$(UAT)" $(UAT_FLAGS)

# ## make ubt
#
# UnrealBuildTool (Build.bat)
#
# Tweak UBT flags with `make ubt UBT_FLAGS="-Help"`
#
# make uat defaults to -Help, unless overriden via UAT_FLAGS="..." on the command line
ubt: UBT_FLAGS ?= -Help
ubt: 
	"$(UBT)" $(UBT_FLAGS)

# ## make build
#
# May want to add --strict-includes with: make build UAT_FLAGS="-StrictIncludes" 

BUILD_DIR = $(WORKDIR)/$(UE_VERSION)

# sleep 2 at the end to workaround a conflicting unreal instance build tool error
$(BUILD_DIR):
	@echo "Building plugin $(PLUGIN_NAME)..."
	powershell -Command "New-Item $(WORKDIR) -ItemType Directory -Force"
	"$(UAT)" BuildPlugin \
		-Plugin="$(PLUGIN_DIR)/$(PLUGIN_NAME).uplugin" \
		-TargetPlatforms=Win64 \
		-Package="$(CWD)/$(BUILD_DIR)" \
		$(UAT_FLAGS)
	powershell -Command 'sleep 2'

build: $(BUILD_DIR)


# ## make build-docker
#
# Build plugin for Linux targets using a docker container
#
# This target runs RunUAT.sh BuildPlugin within a container for target unreal
# version (5.3, 5.4, 5.5, ...) using the current working directory as a
# workspace, and mounted in the container as /workspace.
#
# Result of the build is put in .work/Linux/$(DOCKER_UE_VERSION) Logs are put
# in .work/Logs/unreal-$(DOCKER_UE_VERSION)-$(PLUGIN_NAME).txt
#
# Ephemeral container, automatically removed via --rm flag with docker run
#
# Note: -Package="" is not bind mounted to prevent uat from trying to copy
# artifacts from other engine version builds when building via docker in
# parallel against all engine version targets (5.3, 5.4, 5.5). Mainly
# interrested in log output and exit code, rather than build artifacts.
DOCKER_TARGETS = build-docker-5.5 build-docker-5.6
build-docker-all: $(DOCKER_TARGETS)

$(DOCKER_TARGETS): DOCKER_UE_VERSION=$(subst build-docker-,,$@)
$(DOCKER_TARGETS):
	@echo "Building plugin $(PLUGIN_NAME) via docker (DOCKER_UE_VERSION: $(DOCKER_UE_VERSION))..."
	powershell -Command "New-Item $(WORKDIR)/Logs -ItemType Directory -Force"
	powershell -Command '\
		docker run --rm --name unreal-$(DOCKER_UE_VERSION)-$(PLUGIN_NAME) ` \
			-v $(CWD):/workspace ` \
			--workdir /workspace ` \
			$(DOCKER_IMG) ` \
			/home/ue4/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh BuildPlugin -Plugin="/workspace/$(PLUGIN_NAME).uplugin" -Package="/tmp/packaged/Linux/$(DOCKER_UE_VERSION)" ` \
			$(UAT_FLAGS) ` \
			| Tee-Object -file $(WORKDIR)/Logs/unreal-$(DOCKER_UE_VERSION)-$(PLUGIN_NAME).txt'

# ## make ci
#
# Runs on continuous integration. Run locally to ensure CI will stay fine.
#
# Runs make build-docker-all with 3 jobs in parallel
ci: clean
	make build-docker-all -j 3
	
# ## make build-docker-all
#
# Runs a docker build for each engine version targets (5.3, 5.4, 5.5)


# ## make logs
#
# Can run this when a docker build is running to see the container logs.
#
# Mostly useful when docker run is run in detached mode (-d), which is not the
# case by default (can tweak the target above)
logs: 
	docker logs -f unreal-$(DOCKER_UE_VERSION)-$(PLUGIN_NAME)

# ## make clean
#
# Clean build artifacts
clean: 
	powershell -Command "if (Test-Path $(WORKDIR)) { Remove-Item -Recurse -Force $(WORKDIR) }"

# ## make clean-binaries
#
# Use sparingly if you want to get rid of plugins Binaries and Intermediate
# folders during dev.
#
# Should only run this while editor is not running, and if you want to do a
# clean build within your IDE.
#
# For Rider specifically (not tested in Visual Studio), Rider will regenerate
# the Intermediate folder as soon as it sees a change to that folder.
clean-binaries:
	@echo "Cleaning Binaries / Intermediate..."
	powershell -Command "& { \
		if (Test-Path $(PLUGIN_DIR)/Intermediate) { Remove-Item -Recurse -Force $(PLUGIN_DIR)/Intermediate } \
		if (Test-Path $(PLUGIN_DIR)/Binaries) { Remove-Item -Recurse -Force $(PLUGIN_DIR)/Binaries } \
	}"

# ## make package
#
# Package the plugin for distribution
#
# Build artifacts will be put in .work/Releases
#
# - Move build directory to .work/Releases
# - Tweak .uplugin VersionName to prefix it with +$(UE_VERSION) (ex: "1.1.0+5.5")
# - Create prebuilt zip (all files included)
# - Create stripped zip (exclude Binaries and Intermediate folders)
package: build
	@echo "Packaging plugin $(PLUGIN_NAME)..."
	powershell -Command "& { \
		New-Item -ItemType Directory -Force -Path "$(WORKDIR)/Releases"; \
		Move-Item -Force "$(BUILD_DIR)" "$(WORKDIR)/Releases/$(PLUGIN_NAME)-$(UE_VERSION)"; \
	}"
	powershell -Command '& { \
		$$filename = "$(WORKDIR)/Releases/$(PLUGIN_NAME)-$(UE_VERSION)/$(PLUGIN_NAME).uplugin"; \
		$$result = jq ".VersionName += """"+$(UE_VERSION)""""" $$filename; \
		echo $$result | Set-Content -Path $$filename \
	}'
	powershell -Command "& { \
		Compress-Archive -Path "$(WORKDIR)/Releases/$(PLUGIN_NAME)-$(UE_VERSION)/*" -DestinationPath "$(WORKDIR)/Releases/$(PLUGIN_NAME)-$(UE_VERSION)-prebuilt-Win64.zip" -Force; \
		Set-Location -Path '$(WORKDIR)/Releases/$(PLUGIN_NAME)-$(UE_VERSION)'; \
		$$filesToZip = Get-ChildItem | Where-Object { \
			$$_.FullName -notmatch 'Binaries|Intermediate'; \
		}; \
		Compress-Archive -Path $$filesToZip.FullName -DestinationPath '$(WORKDIR_ABSOLUTE)/Releases/$(PLUGIN_NAME)-$(UE_VERSION).zip' -Force; \
	}"

# ## make release
#
# Package the plugin for distribution on 5.3, 5.4, 5.5
release:
	make package UE_VERSION=5.7

# Help
help:
	@echo Available targets:
	@echo.
	@echo    uat                - Invokes RunUAT.bat with UAT_FLAGS (Default: -Help, Current Value: $(UAT_FLAGS))
	@echo                         Example: make uat UAT_FLAGS="BuildPlugin -Help"
	@echo    ubt                - Invokes Build.bat with UBT_FLAGS (Default: -Help, Current Value: $(UBT_FLAGS))
	@echo                         Example: make ubt UBT_FLAGS="-Help"
	@echo    build              - Build the plugin
	@echo    build-docker-XX    - (Where XX is 56 for engine 5.6 version) Build the plugin in a docker container for Linux targets
	@echo                         DOCKER_IMG: $(DOCKER_REGISTRY):dev-slim-5.6
	@echo    clean              - Clean build artifacts
	@echo    package            - Package the plugin for distribution
	@echo    release            - Package the plugin for distribution for last 3 engine version (update target on engine upgrade) 
	@echo    test               - Bump .uplugin files version to a new VERSION
	@echo    help               - Show this help message
	@echo.
	
.PHONY: all uat ubt ci build-docker logs clean package release help

