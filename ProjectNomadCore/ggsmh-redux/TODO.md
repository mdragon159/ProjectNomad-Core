Just keeping track of miscellaneous todo items for later:

### Goal:

Get this working ASAP with main project and EOS

### Will do
Priority:
- Replace/remove platform-specific code
- (Replace network layer with an EOS compatible abstraction, but work at this layer 
  likely already done)
- Replace or remove assert statements
- Replacing logging with own ILogger interface
- Replace time retrieval with own utility function

Medium priority:
- Remove `GGPO_API` macros
- Remove `__cplusplus` macros
- Remove all `_WIN32` usages

For style (lower priority):
- Remove underscore prefix for member variables
- Remove all `typedef` usages
- Put return type and function name on same line everywhere

### Maybes
- Perhaps remove all anonymously defined structs. Eg, `typedef enum {} varName` or 
  `typedef strut {} varName`
- Remove all `__cdecl` usages
- Remove all `(void)` function param lists
- Remove (practically) all macros and defines

### Much further in future
- Unit tests
- Wrap everything in a namespace
- Add comments to everything. Cuz what the heck does much of this stuff do
- Once sufficiently rewritten, rebrand off of GGPO for variables but add additional explicit 
  credit in README.md