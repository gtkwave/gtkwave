# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'GTKWave'
copyright = '2023, GTKWave Authors'
author = 'GTKWave Authors'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'myst_parser',
    'sphinx_design',
    'sphinx.ext.githubpages'
]

templates_path = ['_templates']
exclude_patterns = []

master_doc = 'index'

# -- Options for MyST ----------------------------------------------------------

myst_enable_extensions = [
    'attrs_inline',
    'colon_fence',
    'deflist',
]

myst_heading_anchors = 3

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_static_path = ['_static']

