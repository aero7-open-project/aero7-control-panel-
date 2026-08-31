# Installation and Updates

Aero7 Control Panel is included with Aero7 and distributed as the
`linux-control-panel` package through the official Aero7 package repository.

## Install

On an Aero7 system with the repository already configured:

```bash
sudo pacman -S linux-control-panel
```

The package installs the application, desktop entries, Start-menu settings
search integration, and required runtime metadata.

## Update

Use **Control Panel > System and Security > Windows Update**, or run:

```bash
sudo pacman -Syu
```

Always perform a complete system update. Partial Arch Linux upgrades are not
supported. If the update includes the kernel, Plasma, Qt, firewall components,
or Control Panel itself, restart Aero7 before testing a reported problem.

## Verify the installed package

```bash
pacman -Qi linux-control-panel
```

The package information shows the installed version and installation date.

## Report an update problem

Include:

- the output of `pacman -Qi linux-control-panel`;
- the exact setting or button involved;
- the exact error text;
- whether Aero7 was restarted after the last full update;
- relevant lines from `/var/log/pacman.log` when installation or updating is
  involved.

Report issues in the
[Control Panel issue tracker](https://github.com/memegeko/aero7-control-panel-/issues).
