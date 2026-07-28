#!/bin/bash

# format and indent cirbuf in Microsoft style
clang-format --style=Microsoft -i cirbuf.h
clang-format --style=Microsoft -i cirbuf.c
