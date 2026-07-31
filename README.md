Road Fighter Remake for GNU/Linux
=================================

This game was developed for the **RETRO-REMAKES REMAKE COMPETITION**
organized by http://www.remakes.org hoping to win a prize!

We hope that you can enjoy playing it as much as we have enjoyed developing it!!

If you want to know more about this game, you can visit:

http://www.braingames.getput.com

After the competition deadline, you will be able to find more information about
the game there.

Disclaimer:
-----------

This is the unofficial remake of Konami **ROAD FIGHTER** which was originally
released in 1985 for the MSX home computer systems.

Making sure that Konami won't sue us, we are telling you that we are not related
to Konami in any way except for liking their excellent games.

Also, this is a not-for-profit remake. So, we don't get any money from remaking
this or any other Konami titles.

Credits:
--------

> Programming: Santi Ontañón
>
> Graphics: Miikka Poikela
>
> Music/SFX: Jorrith Schaap
>
> Beta Testing: Jason Eames, Miikka Poikela, Jorrith Schaap, Santi Ontañón

Special Thanks To:
------------------

 * The Retro Remakes crew, for organizing this wonderful competition!
 * Konami, for releasing the original game!
 * Lars the 18TH, for the gamemap picture
 * Manuel Bilderbeek, for the Linux port

**Necessary dependencies**
--------------------------

The game was ported from SDL2 to SDL3. You will need to have the following
dependencies installed:

**SDL3, SDL3_image, SDL3_ttf, SDL3_mixer, ttf-dejavu**

On Debian/Ubuntu you can get all necessary packages by typing:

    # apt install gcc libsdl3-dev libsdl3-image-dev libsdl3-ttf-dev libsdl3-mixer-dev make ttf-dejavu

On Fedora (`SDL3_mixer` isn't packaged yet at the time of writing, so it's
left out below — see https://bugzilla.redhat.com/show_bug.cgi?id=2454358):

    # dnf install gcc-c++ SDL3-devel SDL3_image-devel SDL3_ttf-devel make dejavu-serif-fonts

On Arch Linux:

    # pacman -S gcc sdl3 sdl3_image sdl3_ttf sdl3_mixer make ttf-dejavu

Compilation:
------------

To build game, do from the source directory:

    $ make clean && make

Once the compilation has finished, execute the created binary:

    $ ./roadfighter

License
=======

> The Road Fighter Remake project as a whole is distributed under the terms of
> the GNU General Public License, version 2 or later, since it contains code
> made available under multiple GPL-compatible licenses.
> We would encourage new contributors to distribute files under this license.

* Copyright (c) 2003-2009 **Santi Ontanon** (Brain Games) - This file was
officially downloaded from http://roadfighter.jorito.net
* Copyright (c) 2017-2019 Work continued by **Carlos Donizete Froes [a.k.a coringao]**
* Copyright (c) 2026 Work continued by **Marcio Carneiro Rodrigues**
