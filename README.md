# Uebermount
This is a small Linux utility program (could've been a gist) that uses overlayfs and namespacing to create the following effect:
  - Visible to the launched process only...
  - A writable folder (the `upperdir`) is overlaid on top of a stack of readable folders (the `lowerdir`) and mounted to a target
  - Such that write operations to the folder by the program affect the writable overlay
  - and read operations read, in the following order:
    - The writable overlay
    - The last readable "lower directory"

This is desirable when, for example, you want to modify a game that requires direct changes to the game files instead of providing a mod API, but don't want other software like Steam to overwrite the changes at the next update.

# Name
"ueber-" from German "über" (over), "mount" from file systems terminlogy.

# Building
```make uebermount```

# License
MIT.

# Contributions
Anything goes.
