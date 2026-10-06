#!/usr/bin/env python3
"""NVIDIA FE Lighting for Linux (GTK4 + libadwaita).

Launcher for the fe_lighting package, lives next to the real file of this script.
`felight` is looked up next to the name this script was started as (or in PATH).
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))

from fe_lighting.app import main

if __name__ == "__main__":
    sys.exit(main(os.path.dirname(os.path.abspath(__file__))))
