---------------------------------------------------------------------------
# Decided Syntax Elements
(Syntax elements that are "decided" - not set in stone, but the best thought so
far.)
---------------------------------------------------------------------------

## EMBEDDED / DELIMITED
|{node ...}         # Inserted Node child - effectively splits text to left and right
:{attribute ...}    # Inserted complex attribute, affecting the parent or root node
!{directive ...}    # Results injected
!{-directive ...}   # Run, but nothing injected
!{'...'}            # (instance of directive for specialized text processing...
                    #  don't yet know if implemented at the udon parsing
                    #  level...)
No such thing as embedded "simple" attribute- grims only.

## SCALARS
### Strings
* LABEL: (Restricted-non-delimited)
    - APPLIES TO: node-name, class-name, attribute-key
    - STOPS ON: whitespace or [|\[.!]
    - (but allows, incedentally, embedded [#:] unless there's a compelling
      reason at some point to dissallow it)
* VALUE: Non-delimited scalars including text
    - APPLIES TO: attribute-values, on-line node data, after '| '
    - STOPS ON: dedented newline or space __plus__ [|#.!:]
* PROTECTED/SHIELDED: parenthasese-delimited label or value
    - APPLIES TO: can be in place of labels or values
    - STOPS ON: last matched close-parenth
    - PROBLEM: if you want an unmatched parenth (like in quotes; ")" for
      example) you're out of luck in label contexts. Use freeform in value
      contexts (possibly need grim attributes if it's an attribute value).
    - Can be interpreted as simple potentially nested tuples or lists
    - And heck, why not. let them start part-way in to a label or value
    - EXAMPLE: (this is all |(part of a) :single (label))
* FREEFORM: any time text starts its own line. Indentation rules still apply.

* NULL: when attribute value or a label is missing, or specified with --|~|null
  (??)

See undecided for other stuff...


## ATTRIBUTES
all ':' attributes are "simple" - label for key & label for value [called into
question- other kinds of scalars...]
':|' attributes (grim attributes) are like nodes except the node-name is in the
     attributes of its parent. Alternately, you can jump into the id part, as
     in: ':[...] ...'

If you need a freeform text value - for example, unbalanced parenths, you need
to use a grim attribute and put the text, indented appropriately, on the next
line.

## GRIM ATTRIBUTES
* Act just like nodes except what would be the "node-name" isn't a child of the
  parent but a unique, unordered attribute of the parent. (As of now, ID isn't
  taken into account).

## INLINE / ONE-LINERS
Strictly determined by indent level. Attributes+ids etc. apply to nearest
beginning of a node (or grim attribute)

|one|two|three    ==>    |{one |{two |{three}}}

|one |two
     |three       ==>    |{one |{two} |{three}}

|one |two
       |three     ==>    |{one |{two |{three}}}
        

Unknown: directives treated like nodes? Probably. (see below)

## NODES
* If a '|' is followed by a space, It no longer deliniates a node, but instead
  deliniates a SIMPLE VALUE
* If a '|' (or grim) is followed directly by an id, class, attribute key, or
  another pipe (node beginning), it is a normal node with a null/anonymous
  name. (used for "implied" nodes, like divs.)
* '|[]' would be an anonymous node with no id or classes - if you want to give
  it immediate simple data, you could do this also: '|| some data' (probably
  the more clear alternative)

## DIRECTIVES
not much decided yet (see below), but indentation-rules etc. will very likely
be exactly the same as nodes.

## MISCELLANEOUS / RECIPES

* ':' allowed in labels (so allowed in attribute keys and node names, etc.)
* To have a key start w/ colon just double it up at the beginning.
* Funny enough- if you want an attribute starting with a colon (ala ruby
  labels) and it's going to be a complex one, it'll start with :|:thekey ...
* Once data text has started on a line, pipes etc. are all treated literally
  like any other text. Need a newline or embedded to get back to structured
  data.
* Markdown-like languages easily implemented with tags, for example, named
  "===", e.g.:     |===[Title of my essay]\n   Hello and welcome...

* Exp

## "ROOT" NODE
* Implied
* ID is file path if applicable
* name is basename of path if applicable
* several :\_\_ attributes for metadata- file access time, etc.
* stuff isn't, by convention, output during conversions

## PARSER
* Warnings w/ severity as a separate structure returned that the implementation
  can decide what to do with.
* Ability to supress specific warning messages



---------------------------------------------------------------------------
# UNDECIDED
quick note on some unresolved issues / un-solidified decisions...
---------------------------------------------------------------------------
## IMPORTANT UNDECIDED
* best, simplest way to have free text where indentation does _not_ apply,
  and/or that protects from udon structures being parsed:
  * yaml-like?
  * heredoc-like?
  * directive only? !{` .... `}




## Philisophical
* Do all these rules and complexities mean that I've absolutely completely
  missed the mark? Or are they elegent, almost ideally optimized compromises
  with the nature of the data being represented?
* Borrow terms from other object notations / markups? (like "tag", "object",
  "class" ...)
* Could it be more unified if everything was a directive and nodes, attributes,
  etc., were syntactic sugar for special kinds of directives? (muahahaha.
  scary.)

[meta - distinguish between:
 * has undefined behavior or puts system in a bad state
 * has defined behavior that possibly isn't ideal
 * has defined behavior that seems to be the desired behavior
]

## LABELS / VALUES
* Delimiters other than () in labels? / simple-values?
* All the '..' vs ".." vs `..` stuff.
* In labels, ever allow/care about escaped values?
* Parse simple scalars?
* Allow embeds? Maybe at least !{...}?

## DIRECTIVES (for text)
* What !{'...'} might or might not be useful for...
* Existence of !" ....  " and how it is parsed, if different (for example, if
  it has a newline that is outdented from the beginning but the closing quote
  hasn't occurred yet, are we still in the string? If so, it has to be parsed
  specially. Unless we allow " to be a special label delimiter- but then all
  the data is simply the "name" of the directive... Maybe follow nesting unless
  you do embedded form and don't require closing quote? and on and on...
* How to easily have embedding-macros for things like "url-encode-this" or
  "don't xml-escape" or even "filter w/ gzip"... filters/nested-filters. Or if
  they even belong in the language outside of directives...

## DIRECTIVES (other)
* Control primitive directives (if/then/else, set/get, foreach...)
* Nested embeddings and what they mean... probably will resolve itself, make
  sense naturally, but still...
* Argument list separate from class/attribute mechanisms for directives?

## Embeds
 #{...}              # Embedded comment. You know you love it. But would then
                       be impossible to use well w/ Ruby.

 nah, we should do |{#        }

 what about a simplification for line-ending comments though?

 and here is some normal text blah blah    #| comment even though I'm in freetext

 meh... probably opens a can of worms...

## NODES
* Allow attributes of a node to continue to be scattered all over the place?
* Is this meaningful:
    |hello
      here is some text
        |am I a node? whose node am I? The texts? |hello's with a warning?
          interpreted simply as text, with warning?
* Class assignments on their own line?

## PARSING
* Error on tab/space mixing?
* "online" mode? that is, "issue" children of the root note as they become
  finalized and flush all on explicit EOF - otherwise returning current
  continuation state?
* In parser, ID and classes become attributes? - implying they get overwritten
  w/ warning when 

## ROOT NODE
* No way (from document) to set classes? - use :class-name true instead?
* No way to set id from source file? use :id (the id) instead?

## GRIM ATTRIBUTES
* ID meaningful? Guess it doesn't hurt- especially if treates as just another
  attribute shorthand...

## SCALARS (simple values)
* Should we actually parse shielded nesting? (since we will need to use
  pushdown automata for nesting anyway...) Have it pre-parsed as s-expression?
* Can a free-form line can be a shielded instead...

### Future scalars ???
(some mentioned above in "IMPORTANT UNDECIDED" - like where udon doesn't get
interpreted, etc.)
* String (other than ID) delimited with (nested) square brackets
* Atom / symbolic / label type
* (future (?)) Quote delimited ... possibly don't need because it would be used
  rarely enough that directives would work fine for it...
* Booleans
* Null
* Dates
* Times
* Intervals
* Numbers
  * Integers
  * (specified w/ Scientific Notation)
  * (bignum?)
  * Hex
  * Octal
  * Binary
  * Floating point
* Regex
* Bitstrings
* Binary

* Simple-list
* Simple-tuple

(plugin for scalars)


