# Target System Plugin

Supported engine versions: 5.1, 5.2, 5.3 (as per [latest release](https://github.com/mklabs/ue4-targetsystemplugin/releases))

An UE4 plugin written entirely in C++ which adds support for a simple Dark Souls inspired Camera Lock On / Targeting system.

It was first developed and tested in Blueprints following the implementation of the people over at Lurendium, with this tutorial series: [Part 1](http://web.archive.org/web/20190115073044/http://www.lurendium.com/target-system-similar-to-dark-souls/), [Part 2](http://web.archive.org/web/20190330014353/http://www.lurendium.com/target-system-similar-dark-souls-blueprint-part-2/), [Part 3](https://web.archive.org/web/20190320143734/http://www.lurendium.com/target-system-similar-to-dark-souls-blueprint-part-3-final/), and then converted and rewritten into a C++ module and plugin.

Get it from the [marketplace](https://www.unrealengine.com/marketplace/en-US/product/target-system-component-plugin) or download the latest `TargetSystem.zip` pre-built plugin zip from the [Release page](https://github.com/mklabs/ue4-targetsystemplugin/releases), and drop the content in your project's `Plugins` folder.

Then, load up Unreal Editor, check the Plugins page and see if `TargetSystem` plugin is enabled.

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
- Adds a Pitch Offset at close range, the greater it is the closer the player gets to the target.

## Usage

Check the [Setup wiki page](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Setup) to get started, the [Configuration](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Configuration) to customize the system's behaviour, or [Blueprint Functions and Events](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Blueprint-Functions-and-Events) to learn more on these.

## Thanks and Credits

- To the people over at Lurendium for their amazing tutorials ([Part 1](http://web.archive.org/web/20190115073044/http://www.lurendium.com/target-system-similar-to-dark-souls/), [Part 2](http://web.archive.org/web/20190330014353/http://www.lurendium.com/target-system-similar-dark-souls-blueprint-part-2/), [Part 3](https://web.archive.org/web/20190320143734/http://www.lurendium.com/target-system-similar-to-dark-souls-blueprint-part-3-final/))

- To Rayziyun on youtube (https://youtu.be/gaULDBoG_oE)

- To Grzegorz Szewczyk for his awesome [Dynamic Targeting component](https://www.unrealengine.com/marketplace/dynamic-targeting)


## License

MIT License.

