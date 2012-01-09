#include "{{parser}}.h"

int {{parser}}_global_error = {{parser|up}}_OK;

{% for f in functions %}
{{f}}

{% endfor %}
