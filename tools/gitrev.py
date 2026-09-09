"""Stamp the build with the commit it came from.

"Which firmware is actually on the chip?" is not a question you can answer by
looking at the machine, and getting it wrong costs a debugging round trip - you
chase a fault that was fixed two commits ago, or you 'fix' code that is already
right. The splash screen says the hash now.

A trailing '+' means the working tree had uncommitted changes when it was built,
which is the other half of the same question.

The commit's date goes with it. Deliberately the commit date rather than the
compile date: two builds of the same source should say the same thing, and "how
old is this firmware" is a fact about the source, not about when someone last
happened to press build.
"""
import subprocess

Import("env")


def _rev():
    try:
        h = subprocess.check_output(
            ["git", "rev-parse", "--short=7", "HEAD"],
            stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return "nogit"
    try:
        if subprocess.call(["git", "diff", "--quiet"],
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL) != 0:
            h += "+"
    except Exception:
        pass
    return h


def _date():
    try:
        return subprocess.check_output(
            ["git", "show", "-s", "--date=format:%d%b%y", "--format=%cd", "HEAD"],
            stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return "--------"


env.Append(CPPDEFINES=[("FW_BUILD", env.StringifyMacro(_rev())),
                       ("FW_DATE",  env.StringifyMacro(_date()))])
