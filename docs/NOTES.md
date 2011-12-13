* inline comments
* no string interpolation / embedded udon
* |kind[what] attributes
    attributes
    children

|dependencies
  |config[/*/gamer-transcoder/*/*]
    |language[python] :version 2.7.2
    |pip[simplejson]  :version 2.2.1
    |pip[amqplib]
    |submodule[...]



-------

implement:
 * toplevel data
 * toplevel comments
 * node:
   * type (simple)
   * id (simple)
   * inline attribute
   * inner-node-inline-comments
   * own-line attributes
   * child data
   * child comments
   * child nodes


node:
 * meta-info
 * attributes (hsearch)
 * children

-------

* current string automatically accumulated
* data "assignment" step
* redo automatic $indent calculation... (?)

