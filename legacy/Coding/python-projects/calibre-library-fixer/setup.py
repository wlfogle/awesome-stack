#!/usr/bin/env python3
"""
Setup script for Calibre Library Fixer
"""

from setuptools import setup, find_packages
from pathlib import Path

# Read the contents of README file
this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text()

setup(
    name="calibre-library-fixer",
    version="2.0.0",
    author="Lou",
    author_email="lou@example.com",
    description="A GUI and CLI tool for organizing Calibre library filenames with standardized naming",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/example/calibre-library-fixer",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: End Users/Desktop",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Multimedia :: Graphics :: Graphics Conversion",
        "Topic :: System :: Archiving",
        "Topic :: Utilities",
    ],
    python_requires=">=3.8",
    install_requires=[
        "PyQt6>=6.0.0",
    ],
    extras_require={
        "dev": [
            "pytest>=6.0",
            "black>=22.0",
            "isort>=5.0",
            "mypy>=0.900",
        ],
    },
    entry_points={
        "console_scripts": [
            "calibre-library-fixer=calibre_library_fixer.cli:main",
            "calibre-fixer-cli=calibre_library_fixer.cli:main",
        ],
        "gui_scripts": [
            "calibre-library-fixer-gui=calibre_library_fixer.gui:main",
            "calibre-fixer-gui=calibre_library_fixer.gui:main",
        ],
    },
    package_data={
        "calibre_library_fixer": ["*.png", "*.ico", "*.desktop"],
    },
    include_package_data=True,
    zip_safe=False,
)
