- [ ] Make sure hsearch-r doesn't free keys unless it really should...
- [ ] Quick rake task (or something) for bumping the version of the library
- [ ] Allow defining your own malloc/free when using the lib
- [ ] Deal with 0 bytes in source (transform in such a way that the parser can
      still use c-strings. Use declarative strings instead/in addition?) -
      both, always record string length and leave it up to library to use it
      when appropriate.
- [ ] Use machine node-name and state names in resulting structure where appropriate:
      * line | column
      * grammar_part | type | state
- [ ] Inline other grammar functions... probably possible (even at construction
      time) for all but recursive ones.
- [ ] Figure out if quickscan etc. can work effectively with heavy UTF-8
- [ ] UTF-8 in matching etc.

- [ ] Inline as much as possible and use -Winline - see http://gcc.gnu.org/onlinedocs/gcc-4.2.1/gcc/Inline.html and http://www.greenend.org.uk/rjk/2003/03/inline.html

- [ ] Place null-terminators only after the parsing pass - that way a character
      can be inspected even if it will later be a terminator for an earlier
      part. (That means that the only time the application will actually have
      to allocate a string is when the same character is marked to be a
      terminator and simultaneously aggregated - which I suspect is quite rare.)
