[![Coverity Scan Status](https://scan.coverity.com/projects/21500/badge.svg)](https://scan.coverity.com/projects/21500)

execas
======

Exec a process as a given user and/or group.

Did you ever wish that you could run `exec` in a shell script with the
ability to change the user id of the program being executed as well?
Or like a `setuid` followed by an `exec`?

Your wish has come true! `execas` does just that.

`sudo -u USER PRG` will run PRG as the given user, but as a child of
`sudo`, which might not be what you want.

How to use execas?
------------------

Example in a bash script:

    #!/bin/bash

    exec execas -u myuser myserver

This will `exec` the program `myserver` (as resolved from the `PATH`
environment variable), as user `myuser`.
