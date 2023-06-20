=Common functionality for QIO Tools components:

  * `Component`: Base for classes which should be printable to C++ streams
    via their `get_class_name()` and `get_status()` methods. Specifically, each
    object derived from Component will be rendered as either

    * ``<class_name>`` (if status is empty), or
    * ``<class_name: status>``
