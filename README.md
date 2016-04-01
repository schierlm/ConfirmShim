Confirm-Shim
============

Proof of concept how I think the ideal interaction with a confirm based shim should look like.


Motivation
----------

On one hand, Secure Boot gets more and more prevalent, not only caused by Microsoft's recent
push of Windows 10. On the other hand, the requirements for signing UEFI boot loaders got a
lot stricter recently: Now you need to be a company (private persons are not eligible) and
own a EV code signing certificate (which costs quite a lot of money). This made services for
the public (hobbyists) to sign either Windows device drivers or UEFI modules virtually
non-existant (For example, [the ReactOS Foundation stopped their driver signing
service](https://www.reactos.org/wiki/Driver_Signing)).

As a consequence, hobbyists who want to bundle system diagnostics software (**not** a Linux
kernel) that can boot from UEFI Secure Boot either use MJG shim or the preloader – requiring
10+ keystrokes on every boot to whitelist the executable, which becomes permanently stored
(unless manually removed) for subsequent boots, thus making the system potentially more
insecure (if anyone ever booted an UEFI shell that way, UEFI shell is whitelisted and could
used by anybody else without confirmation?). The only alternative is to just not support
Secure Boot at all, but have the user go into the firmware setup and disable Secure Boot
(requiring even more keystrokes and sometimes quite some delay for a second POST after the
setting is changed) - and usually forget (or cannot be bothered) to enable it again afterwards,
therefore also resulting in less security overall.

A solution for this problem, in my opining, would be to have a simple shim loader that requires
human interaction before every boot, and then (without storing anything) whitelists all
executables for the current boot. To make the message more appealing, a custom message should
be embeddable inside the confirmation screen, which is (by design) untrusted, but I think
careful wording around the message could compensate for that.


Example screenshot
------------------

![Screenshot](/screenshot.png)


Usage
-----

In fact, in its current state this project cannot be used at all - unless somebody steps
up and helps with reviewing/signing it. (If you know/are anyone who could help, feel free to
[get in touch with me](mailto:schierlm@gmx.de)).

If you want to have a look at the currrent state (or review its code) anyway, it is compiled
with Visual Studio 2015 Community (Update 2). Note that you may have to edit
and save the debugging options (command and arguments) so that they get moved to the
.user file for you, so that debugging (using QEMU) works.

There are three build configurations provided. "Debug" and "Release" are as expected, and
"Prototype" can be used to bypass the Secure Boot check, resulting in a prompt even on systems
that have Secure Boot disabled.

Or just build in release mode and add to a USB key to test on a real device :-)

Next to boot<arch>.efi, add two files:

- boot<arch>.efi-description.txt
- boot<arch>.efi-confirmed.efi


The description (which has to be stored as UTF-16 LE) is shown, and the confirmed.efi is
called once confirmed.


If you only want to have a look at the compiled result, extract the files from the binary release to
`/efi/boot` on your favourite UEFI-bootable USB key (which includes the Linux Foundation's preloader
and an UEFI shell as examples). In case Secure Boot is not enabled, replace loader.efi by
prototype.efi, else whitelist loader.efi from the preloader.