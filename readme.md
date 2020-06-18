# Target System Plugin

An UE4 plugin written entirely in C++ which adds support for a simple Dark Souls inspired Camera Lock On / Targeting system.

It was first developed and tested in Blueprints following the implementation of the people over at [Lurendium](http://www.lurendium.com), with this tutorial series: [Part 1](http://www.lurendium.com/target-system-similar-to-dark-souls/), [Part 2](http://www.lurendium.com/target-system-similar-dark-souls-blueprint-part-2/), [Part 3](http://www.lurendium.com/target-system-similar-to-dark-souls-blueprint-part-3-final/), and then converted and rewritten into a C++ module and plugin.

## Features

- Customizable with a set of options that can be overridden in Blueprints.
- Easy setup: only one Actor component to attach and a minimum of one functions to bind to input.
- Target closest enemy (Pawns by default, customizable with TargetableActors UPROPERTY).
- Break on Line of Sight when getting behind an object.
- Break Target when getting outside minimum distance to enable.
- Simple TargetLockedOn Widget included, can be customized / overridden.
- Option to control character rotation when locked on.
- Switch to new target with axis input (on mouse / gamepad axis movement).
- Two Blueprint implementable events on component on Target Locked On and Off.

## Installation

Download the latest `TargetSystem.zip` pre-built plugin zip from the [Release page](https://github.com/mklabs/ue4-targetsystemplugin/releases), and drop the content in your project's `Plugins` folder. Then, load up Unreal Editor, check the Plugins page and see if `TargetSystem` plugin is enabled.

## Setup

Check the [Setup and Installation wiki page](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Setup-and-Installation).

## Options

- `float MinimumDistanceToEnable`: The minimum distance to enable target locked on.
- `TSubclassOf<AActor> TargetableActors`: The AActor Subclass to search for targetable Actors.
- `TSubclassOf<UUserWidget> TargetLockedOnWidgetClass`: The Widget Class to use when locked on Target. If not defined, will fallback to a Text-rendered widget with a single O character.
- `float TargetLockedOnWidgetDrawSize`: The Widget Draw Size for the Widget class to use when locked on Target.
- `float BreakLineOfSightDelay`: The amount of time to break line of sight when actor gets behind an Object.
- `float StartRotatingThreshold`: Lower this value is, easier it will be to switch new target on right or left.
- `bool ShouldControlRotationWhenLockedOn`: Whether or not the character rotation should be controlled when Target is locked on. If true, it'll set the value of bUseControllerRotationYaw and bOrientationToMovement variables on Target locked on / off. Set it to true if you want the character to rotate around the locked on target to enable you to setup strafe animations.
- `FVector TargetLockedOnWidgetRelativeLocation`: The Relative Location to apply on Target LockedOn Widget when attached to a target.

## Blueprint Functions / Events

- `void TargetActor()`: Function to call to target a new actor.
- `void TargetActorWithAxisInput(float AxisValue)`: Function to call to target a new actor with Axis input, on right or left.
- `OnTargetLockedOff(AActor* TargetActor)`: Called when a target is locked off, either if it is out of reach (based on MinimumDistanceToEnable) or behind an Object.
- `OnTargetLockedOn(AActor* TargetActor)`: Called when a target is locked on.

## Thanks and Credits

- To the people over at [Lurendium](http://www.lurendium.com) for their amazing tutorials ([Part 1](http://www.lurendium.com/target-system-similar-to-dark-souls/), [Part 2](http://www.lurendium.com/target-system-similar-dark-souls-blueprint-part-2/), [Part 3](http://www.lurendium.com/target-system-similar-to-dark-souls-blueprint-part-3-final/))

- To Rayziyun on youtube (https://youtu.be/gaULDBoG_oE)

- To Grzegorz Szewczyk for his awesome [Dynamic Targeting component](https://www.unrealengine.com/marketplace/dynamic-targeting)



## License

MIT License.

