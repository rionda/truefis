# From http://code.activestate.com/recipes/576755/
# by Gabriel Genellina
# based on Recipe 466302: Sorting big files the Python 2.4 way
# by Nicolas Lehuen
#
# Modified for Python 3 and use of tempfile by Matteo Riondato
#
#Copyright (c) 2009, Nicolas Lehuen, Gabriel Genellina
#Copyright (c) 2012, Matteo Riondato <matteo@cs.brown.edu>

#Permission is hereby granted, free of charge, to any person obtaining a copy of
#this software and associated documentation files (the "Software"), to deal in
#the Software without restriction, including without limitation the rights to
#use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
#the Software, and to permit persons to whom the Software is furnished to do so,
#subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
#FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
#COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import collections, heapq, os, tempfile
from itertools import islice, cycle

Keyed = collections.namedtuple("Keyed", ["key", "obj"])

def merge(key=None, *iterables):
    # based on code posted by Scott David Daniels in c.l.p.
    # http://groups.google.com/group/comp.lang.python/msg/484f01f1ea3c832d

    if key is None:
        keyed_iterables = iterables
    else:
        keyed_iterables = [(Keyed(key(obj), obj) for obj in iterable)
                            for iterable in iterables]
    for element in heapq.merge(*keyed_iterables):
        yield element.obj


def batch_sort(input, output, key=None, buffer_size=32000, tempdirs=None):
    if tempdirs is None:
        tempdirs = []
    if not tempdirs:
        tempdirs.append(tempfile.gettempdir())

    chunks = []
    chunks_names = []
    try:
        with open(input,'rt',64*1024) as input_file:
            input_iterator = iter(input_file)
            for tempdir in cycle(tempdirs):
                current_chunk = list(islice(input_iterator,buffer_size))
                if not current_chunk:
                    break
                current_chunk.sort(key=key)
                (tmpfile_handle, tmpfile_name) = tempfile.mkstemp(prefix="exso", dir=tempdir, text=True)
                output_chunk = os.fdopen(tmpfile_handle, 'w+t', 64*1024)
                chunks.append(output_chunk)
                chunks_names.append(tmpfile_name)
                output_chunk.writelines(current_chunk)
                output_chunk.flush()
                output_chunk.seek(0)
        with open(output,'wt',64*1024) as output_file:
            output_file.writelines(merge(key, *chunks))
    finally:
        for i in range(len(chunks)):
            try:
                chunks[i].close()
                os.remove(chunks_names[i])
            except Exception:
                pass


if __name__ == '__main__':
    import optparse
    parser = optparse.OptionParser()
    parser.add_option(
        '-b','--buffer',
        dest='buffer_size',
        type='int',default=32000,
        help='''Size of the line buffer. The file to sort is
            divided into chunks of that many lines. Default : 32,000 lines.'''
    )
    parser.add_option(
        '-k','--key',
        dest='key',
        help='''Python expression used to compute the key for each
            line, "lambda line:" is prepended.\n
            Example : -k "line[5:10]". By default, the whole line is the key.'''
    )
    parser.add_option(
        '-t','--tempdir',
        dest='tempdirs',
        action='append',
        default=[],
        help='''Temporary directory to use. You might get performance
            improvements if the temporary directory is not on the same physical
            disk than the input and output directories. You can even try
            providing multiples directories on differents physical disks.
            Use multiple -t options to do that.'''
    )
    parser.add_option(
        '-p','--psyco',
        dest='psyco',
        action='store_true',
        default=False,
        help='''Use Psyco.'''
    )
    options,args = parser.parse_args()

    if options.key:
        options.key = eval('lambda line : (%s)'%options.key)

    if options.psyco:
        import psyco
        psyco.full()

    batch_sort(args[0],args[1],options.key,options.buffer_size,options.tempdirs)

