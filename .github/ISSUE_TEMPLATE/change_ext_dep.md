---
name: Change - New external dependency
about: Add external library, device driver, sysfs feature, or command line tool
title: 'Add new external dependency: [...]'
labels: task
assignees: ''

---
> estimate 1
Parent [feature/story]: #XXXX

Done criteria:

- [ ] Use of interface exposed by external dependency is compatible
      with the BSD 3 clause license
- [ ] Shared object dependencies have been added to the build system
      in this or a prior PR
- [ ] Dependency has one or more last-level-abstraction classes if
      used in C++
- [ ] Dependency may have last-level-abstraction when used in Python
      if this enables testing
- [ ] All existing features continue to work when dependency is not
      available
- [ ] [...]
- [ ] [...]
