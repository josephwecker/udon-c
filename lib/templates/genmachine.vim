" Vim syntax file
" Language:        genmachine-spec
" Maintainer:      Joseph Wecker
" Latest Revision: 2012-01-13
" License:         None, Public Domain
"
" Newer genmachine mode for (g)vim, more or less from scratch.
"
" TODO:
"

if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

set ambiwidth=double


syn match gmCommentary    /^[ \t]*[^| \t].*$/

syn match gmEnumVal      /|[A-Z][^|]*/ contains=gmDelims
syn match gmStructMember /|[a-z][^|]*/ contains=gmDelims,gmTypes,gmType,gmVariable

syn match gmNamed        /|[^ \[]\+[ \t]*\[[^\]]*\]/ contains=gmDelims,gmName,gmStructure,gmStateDef,gmFunctionDef
syn match gmCase         /|\(c[ \t]*\[[^\]]*\]\|default\)/ contains=gmChars,gmDelims1
syn match gmIf           /|\(if\|elsif\|else\|endif\)[ \t]*\(\[[^\]]*\]\)\?/ contains=gmDelims,gmStatement
syn match gmEOF          /|eof[ \t][^|]*/ contains=gmDelims
syn match gmErr          /|err[ \t][^|]*/ contains=gmErrkw
syn match gmErrkw        /|err/ contains=gmDelims
syn match gmStructure    /enum\|struct/ contained
syn match gmStateDef     /state/ contained
syn match gmFunctionDef  /function/ contained

syn match gmDirective    /|\(parser\|entry-point\|return\|>>\)/ contains=gmDelims

syn match gmName         /\[[^\]]*\]/ contained contains=gmDelims2
syn match gmChars        /\[[^\]]*\]/ contained contains=gmDelims1,gmSpecialChars
syn match gmSpecialChars /\\t\|\\n\|<LBR>\|<PIPE>/ contained

syn match gmDelims       /[|\/]/ contained
syn match gmDelims1      /[|]/ contained
syn match gmDelims2      /[\[\]:\.]/ contained
syn match gmDelims3      /[\[\]]/ contained

syn match gmLocation     /\/[^ :]*\(:[^ \.]*\(\.[^ ]*\)\?\)\?\|:[^ \.]*\(\.[^ ]*\)\?/ contains=gmDelims,gmState,gmSubstate

syn match gmState        /:[^ \.]*/ contained contains=gmDelims2
syn match gmSubstate     /\.[^ ]*/ contained contains=gmDelims2
syn match gmTypes        /STRING\|Z+\|LIST\|DICT/

syn match gmStatement    /\[[^\]]*/ contained contains=gmDelims2,gmVariable,gmBooleanOps,gmInt,gmConstant

syn match gmVariable     /[a-z][a-z0-9_\.]*/ contained contains=gmContsts,gmConstant
syn match gmConstant     /[A-Z][A-Z0-9_]*/ contained
syn match gmType         /[A-Z]\+[a-z]\+[A-Za-z0-9_]*/ contained
syn match gmBooleanOps   /==\|>=\|<=\|!\|!=/ contained
syn match gmOperator     /=\|-\|+\|+=\|-=/
syn match gmSpecialOps   /<<\|->/
syn match gmInt          /[0-9]\+/

syn match gmSubstateDef  /|\.[a-z0-9_-]\+/ contains=gmDelims
syn match gmCommand      /|\([ \t]\+[^|]*\|$\)/ contains=gmDelims,gmVariable,gmSpecialOps,gmOperator,gmInt,gmConsts,gmLocation,gmConstant,gmType

syn keyword gmConsts     S COL C MARK MARK_END


hi def link gmCommentary Comment
hi def link gmSubstateDef SpecialComment
hi def link gmStructure  Structure
hi def link gmDirective  PreProc
hi def link gmDelims     Delimiter
hi def link gmDelims2    Delimiter
hi def link gmDelims3    Delimiter
hi def link gmName       Identifier
hi def link gmLocation   Define
hi def link gmState      Macro
hi def link gmSubstate   PreCondit
hi def link gmTypes      Type
hi def link gmStateDef   Special
hi def link gmFunctionDef Function
hi def link gmCase        Label
hi def link gmIf          Conditional
hi def link gmChars       String
hi def link gmSpecialChars Character
hi def link gmVariable    Identifier
hi def link gmBooleanOps  Float
hi def link gmInt         Number
hi def link gmOperator    Operator
hi def link gmSpecialOps  SpecialChar
hi def link gmCommand     Statement
hi def link gmConsts      Constant
hi def link gmConstant    Keyword
hi def link gmType        StorageClass
hi def link gmEnumVal     Keyword
hi def link gmEOF         Tag
hi def link gmErrkw       Exception
hi def link gmErr         String

let b:current_syntax = "genmachine"
