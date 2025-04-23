

* Please always read this file and make sure you do all these things on every instruction that I give.
* The terminal is Powershell 7, make sure to use the correct Powershell 7 syntax when executing commands
* The build can be run via the config in the tasks.json file. Thats the definitive place to look. Don't try and run the build in ways not specified there.
* The build should always be run after code changes to make sure there are no build errors. If there are errors, your work is not done, iterate to fix them before declaring that you're done.
* Do not just add comments and expect things to change, make sure you've actually made meaningful changes in code.
* Do not make unrelated changes to comments in files that you're working on. Leave existing comments alone unless they are out of date, wrong, or I expressly ask you to remove them.
* Remember to add/update things in the CMakeLists when you're creating, moving, or deleting files.
* When you move files, always make sure that anything that depends on them is updated to use the new location
* When you move files, make sure the old ones are deleted.
* Look for duplication in file/class names and their namespaces. Classes like Text that are in the Shapes folder should be called Text, not TextShape becuase its redundant. There may be exceptions to this rule, just ask if you're worried about namespace collisions due to short names.
* When removing code from files, remember to check for removing imports that are no longer being used
* Please put the header files for C++ classes in the same directory as the class, not in a seperate /include directory.
* When building the application with new changes, also make sure there are no warnings introduced. If there are, treat them like errors and iterate to fix.