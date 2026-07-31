# Personal Coding Style

## Enforcement

`clang-format` (with the repository `.clang-format` config) is the enforcement
tool. Code must pass:

    clang-format --dry-run --Werror src/*.c src/*.h

Deliberate column alignment that clang-format cannot reproduce must be wrapped
in `// clang-format off` / `// clang-format on` regions.

## Visual Conventions

### Naming Conventions

 - Constants (including enum values) use UPPER_SNAKE_CASE, e.g. `MY_CONSTANT`
 - Macros use UPPER_SNAKE_CASE, e.g. `MY_MACRO()`
 - Types use PascalCase, e.g. `MyStruct`
 - Functions use snake_case, e.g. `my_function()`
 - Variables use camelCase, e.g. `myVariable`
 - Struct and union members use camelCase, e.g. `myMember`
 - Functions are grouped by module with a `module_` prefix,
   e.g. `string_printf`, `hashmap_find`, `list_append`, `gprint`
 - X-macros use the single-letter `X` name

### Line length

Lines should not exceed 120 characters.

### Brace Placement

K&R Style:

    if (something) {
        ...
    } else {
        ...
    }

### Case Placement

Case statements are indented once:

    switch (something) {
        case x:
            ...
        case y:
            ...
        default:
            ...
    }

### Spacing

Spaces between statements and parentheses:

    if (something)
    while (something)

Spaces around binary operators:

    x + y
    x *= y
    arr[i - 1]

Spaces after ',':

    foo(1, 2, 3)

Spaces after cast:

    (int) x;

No spaces in parentheses:

    (int) (x + foo(y))

NOT:

    ( int ) ( x + foo( y ) )

Function and macro calls have no space before the parenthesis:

    foo(1)
    assert(x)

### Indentation

Indentation is done with 4 spaces only, never tabs.

### Single line statement bodies

A single statement body goes on the same line, without braces. `if`/`else`
bodies are always split onto their own line:

    if (check(x)) return 1;

    while (i < 10) i++;

    if (check(x))
        return 1;
    else
        return 0;

If one of the `if`/`else` branches has more than a single statement, both use
braces:

    if (check(x)) {
        return 1;
    } else {
        foo();
        return x;
    }

Functions bodies are always on a new line:

    int foo() {
        return 1;
    }

### Column Alignment

Consecutive enum values, switch-case statements, and keyword tables may be
column-aligned for readability. Blocks whose alignment clang-format cannot
reproduce exactly are wrapped in `// clang-format off` / `// clang-format on`.

### Pointers and References

The `*` (and `&`) binds to the variable name:

    Type *name
    int *a, *b

### Header Guards

Headers use `#ifndef` guards matching the file name:

    #ifndef FILENAME_H
    #define FILENAME_H
    ...
    #endif // FILENAME_H

### Comments

Use `//` for line comments. Trailing comments are aligned on the column:

    int foo = 1;  // trailing comment
    long bar = 2; // aligned with the comment above
