"""
Tools used during configuration and build process
(most of them used in CMake files)

This file is to be copied into CMAKE_BINARY_DIR/share using configure_file
(it needs preprocessing, see global variables on top.)
"""

""" Siconos is a program dedicated to modeling, simulation and control
 of non smooth dynamical systems.

 Copyright 2018 INRIA.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
"""

import glob
import os
import subprocess
from doxy2swig import Doxy2SWIG

def parse_cmake_list(var):
    """Transform cmake list-like variables 
    into python list.
    
    Parameters
    ----------
    var : string
        like "a;b;c"
        

    Returns python list
    
    If var is already a list, does nothing.

    Example::

        a = parse_cmake_list("var1;var2;var3;")
        # --> a = ['var', 'var2', 'var3']

    """
    if isinstance(var, list):
        return var
    if var != "":
        res = list(set(var.split(';')))
        # list/set stuff to avoid duplicates
        # remove empty strings to avoid '-I -I' things leading to bugs
        if res.count(''):
            res.remove('')
        return res
    else:
        return []



def parse_doxygen_config(filename):
    """Read doxygen config into a python dictionnary
    
    Parameters
    ----------
    filename : string
       full path to doxygen file
    
    Returns a python dictionnary
    """
    result = {}
    with open(filename) as ff:
        # remove comment lines
        conf = [n.strip() for n in ff.readlines() if (not n.startswith('#') and not n.startswith('\n'))]
        for d in conf:
            if d.count('=') > 0:
                res = d.split('=')
                result[res[0].rsplit()[0]] = res[1]
                backup = res.copy()
            else:
                result[backup[0].rsplit()[0]] += d
    return result
    
def get_xml_files(header_name, xml_path, case_sense_names=True):
    """Return all xml files generated by doxygen from a given header file.
    and found in a given path.

    Parameters
    ----------
    header_name : string
        header file.
    xml_path : string
        absolute path to xml files.
    case_sense_names : bool
        false if xml output files names are lower case only

    Returns a list of xml files.
    
    Notes
    -----
 
    * This function takes into account the value of CASE_SENSE_NAMES parameter
    in doxygen config.
    * It looks for '*classNAME.xml', '*structNAME.xml' and 'NAME_8h*.xml'
    name being the input header name (without ext).

    """
    # Get filename without extension
    fnwe = os.path.basename(header_name).split('.')[0]
    # Replace _ with __ in filename
    fnwe = fnwe.replace('_', '__')
    if not case_sense_names:
        fnwe = replace_uppercase_letters(fnwe)
    # Look for 'class' files
    classfiles = glob.glob(os.path.join(xml_path, '*class' + fnwe + '.xml'))
    # Look for 'struct' files
    structfiles = glob.glob(os.path.join(xml_path, '*struct' + fnwe + '.xml'))
    # Look for '8h' (?) files
    files8h = glob.glob(os.path.join(xml_path, fnwe + '_8h*.xml'))
    allfiles = classfiles + structfiles + files8h
    return allfiles



def xml2swig(header_name, xml_path, swig_path, case_sense_names):
    """For a given header file, create swig (.i) file using 
    xml outputs from doxygen.

    Parameters
    ----------
    header_name : string
        Name of the header file (h or hpp)
    xml_path : string
        absolute path to xml files.
    swig_path : string
        Absolute path to swig outputs
    case_sense_names : bool
        false if xml output files names are lower case only
   
     Notes
     -----
     * This function takes into account the value of CASE_SENSE_NAMES parameter
     in doxygen config.
     * .i file will be named as the xml input file, with .i as ext.

    """
    allfiles = get_xml_files(header_name, xml_path, case_sense_names)
    
    if not os.path.exists(swig_path):
        os.makedirs(swig_path)
    for f in allfiles:
        # set output filename == xml file without extension + .i
        outputname = os.path.basename(f).split('.')[0] + '.i'
        outputname = os.path.join(swig_path, outputname)
        p = Doxy2SWIG(f)
        # with_function_signature = options.f,
        # with_type_info = options.t,
        # with_constructor_list = options.c,
        # with_attribute_list = options.a,
        # with_overloaded_functions = options.o,
        # textwidth = options.w,
        # quiet = options.q)
        p.generate()
        p.write(outputname)
        msg = "Docstrings generation for "
        msg += header_name + " from " + f
        #print(msg)



def build_docstrings(headers, component_name, doxygen_config_filename, swig_path):
    """Create docstrings (doxy2swig) in swig files from xml (doxygen) generated
    from headers.

    Parameters
    ----------

    headers : list (cmake like)
         headers files to parse
    component_name : string
         component (numerics, kernel, ...) of interest
    doxygen_config_filename : string
         name (full path) of the doxygen configuration file
    swig_path : string
         path to swig outputs

    Note
    ----
    * all swig files will be genereted into swig_path/tmp_component_name directory
    and concatenated into component_name-docstrings.i that will be the file really
    used by swig.
    """
    doxyconf = parse_doxygen_config(doxygen_config_filename)
    case_sense_names = doxyconf['CASE_SENSE_NAMES'].find('YES') > -1 
    xml_path = os.path.join(doxyconf['OUTPUT_DIRECTORY'].lstrip(), doxyconf['XML_OUTPUT'].lstrip())
    tmp_path = os.path.join(swig_path, 'tmp_' + component_name)
    headers = parse_cmake_list(headers)
    for hfile in headers:
        xml2swig(hfile, xml_path, tmp_path, case_sense_names)

    outputfile = os.path.join(swig_path, component_name + '-docstrings.i')
    swigfiles = glob.glob(os.path.join(tmp_path, '*.i'))
    with open(outputfile, 'w') as outfile:
        for fname in swigfiles:
            with open(fname) as infile:
                for line in infile:
                    outfile.write(line)
    msg = 'Generates file ' + outputfile + ' for doctrings in swig.'
    print(msg)
    
def replace_uppercase_letters(filename):
    """Replace uppercase letters in a string
    with _lowercase.
    
    e.g. : TimeStepping --> _time_stepping

    This is useful to postprocess filenames from xml-doxygen outputs
    and feed them to doxy2swig, even when CASE_SENSE_NAMES = NO
    in doxygen config.

    Usage:

    result = replace_uppercase_letters(input)
      
    """
    r = []
    for c in filename:
        # l being: last character was not uppercase
        newc = c
        if c.isupper():
            newc ='_' + c.lower()
        r.append(newc)
    return ''.join(r)


