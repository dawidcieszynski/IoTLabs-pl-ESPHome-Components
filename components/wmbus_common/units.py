from pathlib import Path
import string

_units_dict = {}


def build_units_dict():
    """Builds a dictionary of units from the units.h file."""
    if not _units_dict:
        with Path(__file__).with_name("units.h").open() as file:
            for line in file:
                if "LIST_OF_UNITS" in line:
                    break

            for line in file:
                if line.strip() == "":
                    break

                line = (
                    line.strip(string.whitespace + "\\")
                    .removeprefix("X(")
                    .removesuffix(")")
                )
                if line:
                    line = line.split(",")
                    if len(line) == 5:
                        cname, lcname, hrname, quantity, explanation = line
                        _units_dict[lcname] = hrname.strip('"')
                        if lcname == 'bar':
                            break

    return _units_dict


def get_human_readable_unit(unit: str):
    return build_units_dict().get(unit.lower(), unit or "?")
