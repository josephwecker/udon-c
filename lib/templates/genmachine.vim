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

syn match gmNamed        /|[^ \[]\+[ \t]*\[[^\]]*\]/ contains=gmDelims,gmName,gmStructure,gmStateDef,gmFunctionDef,gmCase,gmIf

syn match gmStructure    /enum\|struct/ contained
syn match gmStateDef     /state/ contained
syn match gmFunctionDef  /function/ contained
syn match gmCase         /c/ contained
syn match gmIf           /if/ contained

syn match gmDirective    /|\(parser\|entry-point\)/ contains=gmDelims

syn match gmName         /\[[^\]]*\]/ contained contains=gmDelims2

syn match gmDelims       /[|\/]/ contained
syn match gmDelims2      /[\[\]:\.]/ contained


syn match gmLocation     /\/[^ :]*\(:[^ \.]*\(\.[^ ]*\)\?\)\?/ contains=gmDelims,gmState,gmSubstate

syn match gmState        /:[^ \.]*/ contained contains=gmDelims2
syn match gmSubstate     /\.[^ ]*/ contained contains=gmDelims2
syn match gmTypes        /STRING\|Z+\|LIST\|DICT/

hi def link gmCommentary Comment
hi def link gmStructure  Structure
hi def link gmDirective  PreProc
hi def link gmDelims     Delimiter
hi def link gmDelims2    Delimiter
hi def link gmName       Identifier
hi def link gmLocation   Define
hi def link gmState      Macro
hi def link gmSubstate   PreCondit
hi def link gmTypes      Type
hi def link gmStateDef   Special
hi def link gmFunctionDef Function
hi def link gmCase        Label
hi def link gmIf          Conditional

let b:current_syntax = "genmachine"
