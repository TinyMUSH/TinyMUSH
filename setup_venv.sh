#!/bin/bash
# Setup Python virtual environment for TinyMUSH development

set -e

VENV_DIR="venv"

if [ -d "$VENV_DIR" ]; then
    echo "Virtual environment already exists in $VENV_DIR"
    echo "To recreate it, remove the $VENV_DIR directory first"
    exit 0
fi

echo "Creating virtual environment in $VENV_DIR..."
python3 -m venv "$VENV_DIR"

echo "Activating virtual environment and installing dependencies..."
source "$VENV_DIR/bin/activate"
pip install --upgrade pip
pip install telnetlib3

echo "Virtual environment setup complete!"
echo ""
echo "To use the virtual environment:"
echo "  source $VENV_DIR/bin/activate"
echo ""
echo "To run regression tests:"
echo "  source $VENV_DIR/bin/activate"
echo "  TINY_USER=\"#1\" TINY_PASS=\"potrzebie\" python scripts/regression_test.py -c scripts/functions_string_remaining.conf"