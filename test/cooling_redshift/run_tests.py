"""Run the linked cooling regression against synthetic, offline table fixtures.

Build using the target documented in README.md. Synthetic tables test indexing
and interpolation; they are not physical cooling tables for a production run.
"""

from array import array
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve()
    base_parameters = (Path(__file__).resolve().parents[1] / "sedov" / "sedov.params").read_text()
    base_parameters += "\n" + "\n".join(f"Softening{kind} 0.01" for kind in
                                              ("Gas", "Halo", "Disk", "Bulge", "Stars", "Bndry")) + "\n"
    with tempfile.TemporaryDirectory(prefix="gizmo-cooling-redshift-") as tmp:
        directory = Path(tmp)
        (directory / "spcool_tables").mkdir()
        # FIRE uses 10 live table entries: electron abundance plus nine metal entries.
        for index in range(49):
            values = array("f", [1.] * (41 * 176))
            values.extend(array("f", [(index + 1) * 1.e-23] * (9 * 41 * 176)))
            with (directory / "spcool_tables" / f"spcool_{index}").open("wb") as output:
                values.tofile(output)
        with (directory / "TREECOOL").open("w") as output:
            for x in (0., 0.125, 0.25, 0.5, 0.75, 1.):
                rate = 10 ** (-12 + x + x*x)
                output.write(f"{x} " + " ".join([f"{rate:.17g}"] * 6) + "\n")
        subprocess.run([str(executable)], cwd=directory, check=True)
        (directory / "output").mkdir()
        for value in (None, "0", "2.5", "-1", "nan", "inf"):
            parameters = directory / "test.params"
            parameters.write_text(base_parameters + ("" if value is None else f"\nCoolingRedshift {value}\n"))
            result = subprocess.run([str(executable), "--parameters", str(parameters)], cwd=directory,
                                    check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            if value in ("-1", "nan", "inf"):
                assert "CoolingRedshift must be finite and non-negative" in result.stdout, result.stdout
                assert "PARAMETER_TEST" not in result.stdout, result.stdout
            else:
                expected = 0 if value is None else float(value)
                assert f"PARAMETER_TEST CoolingRedshift={expected:g}" in result.stdout, result.stdout
        print("PASS: parameter parsing, omitted default, explicit zero, fractional redshift and invalid inputs")


if __name__ == "__main__":
    main()
