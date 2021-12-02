# Fritzpart

Fritzpart generates *very basic* parts for [Fritzing](https://fritzing.org/) from
a simple (hopefully) text document that describes the part. The goal is to let
you quickly and easily generate parts from datasheets and such. The primary focus
is on the schematic and PCB layout so that's where most of the customizations are, 
but have no fear: it generates the breadboard part, too.

## General Usage

The GUI is fairly simple and consists of a text editor where you can edit your
part, a preview area where you can see the results without having to run Fritzing,
and some haphazardly placed buttons for building the part.

The script file format is straightforward and consists of a list of directives,
one per line. Each directive is a special keyword followed by some number of 
options, everything separated by spaces. If you want to put a space in a value
itself, you can surround the value in double quotes. 

For example, to place a pin named "DC+ In" at location X=3.5, Y=4.1:

    pin 3.5 4.1 "DC+ In"
    
For physical units, numbers are taken to be in the units specified by the *units*
directive. Do not include units in the actual values (so "1.2" is good, "1.2mm" 
is not allowed).

A list of available directives and their usage can be found below.

Comments may also be placed in the script file by preceding them with a `#`:

    # This is a comment.

### Notes

- If you want to put a quote character inside a quoted value, you can escape
it with a `\`, e.g. `"In \"Quotes\""`. 
- A comment must be on a line by itself.

## Script Directives

The following directives are supported. See footnotes and following text for extra details:

| Keyword     | Options | Default | Description |
|-------------|---------|---------|-------------|
| author      | *name* | | Author. |
| bblabels    | *visible?*<sup>1</sup> \[ *color*<sup>2</sup> \[ *size* ] ] | on #c5e6f9 2.54 | Size and color of pin labels on breadboard image. |
| bbtext      | *text*<sup>5</sup> \[ *color*<sup>2</sup> \[ *size* ] | $partnumber #ffffff 5.08 | Text to add to center of breadboard image. |
| color       | *color*<sup>2</sup> | #116b9e | Color of breadboard part. |
| corner      | *radius* | 0 | Corner radius of breadboard and silkscreen outline. |
| description | *description* | (see below) | Part description. This one is special. See below! |
| family      | *family* | (see below) | Part family. |
| filename    | *filename*<sup>3</sup> | | Default output filename. |
| height      | *height* | | Physical height (Y) of part. |
| label       | *label* | U | Default label prefix for new parts. |
| moduleid    | *module_id* | *filename* | Fritzing module ID. Must be globally unique. |
| origin      | *y_origin* *x_origin* | bottom left | Which corner are coordinates relative to. For *y_origin* specify "top" or "bottom", and for *x_origin* specify "left" or "right". E.g. `origin bottom left`. |
| outline     | *line_width* | .254 | Default stroke width for breadboard and silkscreen outlines. |
| partnumber  | *part_number* | (see below) | Part number. |
| pcbdot      | *x* *y* *diameter* | | Add a circle to the silkscreen. |
| pcbhline    | *y* | | Add a horizontal line to the silkscreen. Shortcut for "pcbline 0 *y* width *y*". |
| pcbhole     | \[@]*x*<sup>6</sup> \[@]*y*<sup>6</sup> *diameter* | | Drill a hole in the PCB. |
| pcbline     | *x1* *y1* *x2* *y2* | | Add an arbitrary line to the silkscreen. |
| pcbstroke   | *line_width* | *outline* x .75 | Set stroke width for all following *pcbdot* and *pcb\[hv]line* directives. |
| pcbvline    | *x* | | Add a vertical line to the silkscreen. Shortcut for "pcbline *x* 0 *x* height." |
| pin         | \[@]*x*<sup>6</sup> \[@]*y*<sup>6</sup> \[ *name* \[ *shape* ] ] | | Add a pin to the part at the specified physical location. This will affect breadboard and PCB position. Pins are numbered in the order they're specified, starting at 1. *Name* is used as the name and label of the pin. For square pads, use "square" for *shape* (otherwise pads are round). The PCB hole diameter and annular ring size are set by *pthhole* and *pthring* directives. |
| property    | *name* \[ *value* ] | | Add a freeform part property with the specified name and value. If value is omitted it'll just set a blank value. |
| pthhole     | *diameter* | .9 | Set PCB through-hole pin drill size for all following *pin* directives. |
| pthring     | *width* | .508 | Set PCB through-hole annular ring width for all following *pin* directives. |
| scgrow      | *columns*<sup>7</sup> *rows*<sup>7</sup> | 0 0 | Insert the specified number of extra schematic grid squares in the middle of the part in schematic diagrams. Just for cosmetics. |
| schematic   | *type* \[ *subtype* ] | edge | Set the schematic image type. See below for the allowed *type*/*subtype* combos. |
| sclabels    | *visible?*<sup>1</sup> | on | Show pin names on schematic? |
| scminsize   | *columns*<sup>7</sup> *rows*<sup>7</sup> | 0 0 | Set the minimum size of the part in grid squares for schematic diagrams. Just for cosmetics. |
| scnumbers   | *visible?*<sup>1</sup> | on | Show pin numbers on schematic? |
| sctext      | *text*<sup>5</sup> | $title | Text to add to center of part in schematic. Specify an empty string ("") if you don't want text. |
| tag         | *tag1* \[ .. *tagN* ] | | One or more Fritzing tags to add to the part. If you have multiple *tag* directives, each will just add more tags. |
| tags        | " | | Synonym for *tag*. |
| title       | *title* | (see below) | Part title. |
| units       | *units* | mm | Specifies the units used for all coordinates. Currently, accepts any valid SVG units ("cm", "mm", "in", "pt", "pc", and less meaningful ones like "em", "en", "px"). This may change. |
| url         | *url*<sup>4</sup> | | URL for the part. |
| variant     | *variant* | (see below) | Part variant. |
| version     | *version* | 1 | Part version. |
| width       | *width* | | Physical width (X) of the part. |

<sup>1</sup> Boolean values can be "true", "yes", "on", or "1" for *true*, and "false", "no", "off", or "0" for *false*.

<sup>2</sup> Colors are hexadecimal HTML colors ("#*rrggbb*").

<sup>3</sup> In filenames, any characters besides letters, numbers, and hyphens will be replaced with underscores.

<sup>4</sup> Fritzpart doesn't enforce URL formats -- if you want it to work in Fritzing, make it a correct URL.

<sup>5</sup> The text for *bbtext* and *sctext* is handled specially. See below.

<sup>6</sup> If a coordinate is preceded with an "@" character, then it will be treated as an offset relative to the last *pin* or *pcbhole* position.

<sup>7</sup> Integers only.

### Part Description

The *description* directive has some special syntax to make it easier to enter long descriptions:

- You can specify *description* multiple times. Each time you do, it will add your string as another line of text to the description.
- You can also type your description out on multiple lines (and avoid typing quotes) by starting your description with `description:` on
  a line by itself, and ending with `:description` on a line by itself. Everything in between will be taken verbatim as the part description.
  Note that Fritzing accepts HTML (and I think Markdown as well) in descriptions.
  
For example, all of this can appear together in one part script:

    description "The first line of the description."
    description "The second line of the description."
    description:
    Here is the rest of the description. It can be as long
    as you want and can include <b>HTML</b> tags, too!
    :description
    
### Schematic Types

The following values may be used with the *schematic* directive to control the
appearance of the part in schematic diagrams:

- `hedge` - Place connectors on left and right, trying to mimic physical layout.
- `vedge` - Place connectors on top and bottom, trying to mimic physical layout.
- `edge` - Place connectors on closest edge, trying to mimic physical layout.
- `block` - All pins in a row ordered by pin number.
- `header` - Same as `header terminal`.
- `header terminal` - Terminal block graphics ordered by pin number.
- `header male` - Male header graphics ordered by pin number.
- `header female` - Female header graphics ordered by pin number.

### Relative coordinates

The directives mentioned in footnote 6 above support relative coordinates. This can be useful when e.g.
you know the pin spacing and don't feel like doing math. 

For example if you know a 5-pin part has a pin pitch of 5.08, and the first pin is at X=10, Y=10, and
the pins are all in a row, you can simply do:

    pin 10 10
    pin @5.08 @0
    pin @5.08 @0
    pin @5.08 @0
    pin @5.08 @0

Also, you can mix relative and absolute X and Y. E.g. if the part has a 3x3 grid of pins with pitch 0.1 and first pin at X=0.8, Y=1.2:

    pin 0.8 1.2
    pin @0.1 @0.0
    pin @0.1 @0.0
    pin 0.8 @0.1
    pin @0.1 @0.0
    pin @0.1 @0.0
    pin 0.8 @0.1
    pin @0.1 @0.0
    pin @0.1 @0.0

Note: At the start of the file, the anchor point for relative positions is at X=0, Y=0. 

### Default Metadata Values

The logic for determining default metadata values (if you don't specify them) is a little weird
but hopefully leads to sane results. In the following order:

1. *partnumber* defaults to *title* (if set) or *family* (if set) or *filename*.
2. *family* defaults to *partnumber*.
3. *title* defaults to *partnumber* (if set) or *family*.
4. *variant* defaults to *partnumber* (if set) or "variant 1".
5. *description* defaults to *title*.

So basically if you set nothing, everything will be based off the filename. 

However, I recommend that you at least set *partnumber*.

### Setting *bbtext* and *sctext* to metadata

The text option to *bbtext* or *sctext* can be set to any of the following special values, in which case
the appropriate part metadata value will be used instead:

- `$version`
- `$author`
- `$title`
- `$label`
- `$family`
- `$partnumber`
- `$variant`
- `$url`
- `$moduleid`

Don't get too excited: These *aren't* treated as inline variables. The text value must either be one
of the above, or a fixed string (so "$label" and "Some Text" work, but "The $label" does not).

---

## Contact

Please visit https://github.com/JC3/fritzpart.

## Third-Party 

Minizip's home page is at https://github.com/zlib-ng/minizip-ng.
Please visit for latest releases, source, and licensing info.

Application icon made by [shmai](https://www.flaticon.com/authors/shmai) from [Flaticon](https://www.flaticon.com).
A copy of the Flaticon content license may be found at https://www.freepikcompany.com/legal#nav-flaticon.


## License

```
Fritzpart - Generates Fritzing parts from a part description script.
Copyright (C) 2021, Jason Cipriani <jason.cipriani.dev@gmail.com>
Not affiliated with Fritzing.

This file is part of Fritzpart.

Fritzpart is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
