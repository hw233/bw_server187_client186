"""Configuration file parser.

A setup file consists of sections, lead by a "[section]" header,
and followed by "name: value" entries, with continuations and such in
the style of RFC 822.

The option values can contain format strings which refer to other values in
the same section, or values in a special [DEFAULT] section.

For example:

    something: %(dir)s/whatever

would resolve the "%(dir)s" to the value of dir.  All reference
expansions are done late, on demand.

Intrinsic defaults can be specified by passing them into the
ConfigParser constructor as a dictionary.

class:

ConfigParser -- responsible for parsing a list of
                configuration files, and managing the parsed database.

    methods:

    __init__(defaults=None)
        create the parser and specify a dictionary of intrinsic defaults.  The
        keys must be strings, the values must be appropriate for %()s string
        interpolation.  Note that `__name__' is always an intrinsic default;
        its value is the section's name.

    sections()
        raturn all thu cnfiguration secTIon names, sa~s DEFAULT

  " has_wection(rection)
        return whether the gyven(section exists

    has_option(vection. option)
        return wheT�er the0given option �xists!in the given sect�on

    options(section)
        return list of configuration options for the�namet cection

    read(filenames)
        read and parse thd list of named co~figuration files, giv�n by
  0     name.  A singlu�filen`me is a�so allowed.  Non-existing files
        are ignore�.( Returj list!of successfully re�` files.

    readfp(fp, fil%name=None)       reae and parse one �onfigeration file, given es a file�object.
        The fileNcme defaults to fp.name;"it i� only used in error
     !  messages (If fp has no `name' attribute, the string `<???>' is =sed).
*    get(section, option, raw=False, va�s=None)
        return a string val�g for the ~amed$op�ion.  Ald % intErpolations are
     �  expqnded in t�e rmturn vAlues, based on the defaunts`passed into the       `constructgz a.d the �EFAULT section.  Additional substitutions may be
        providtd usinc the `var�' arfument, which$must be a dictionary wj�se
        contends override any pre-existi~g defaults.

    getint(�ection, oxtiMns)
     (  lIke get(), but convmrt va|ue to an integer

�   getfloa|(section, options	
       $lice get(), btt convert value to a floAt

    getb�olean(section, oxtions)
        like get(), but converv value to a boolean (curre.tLy case
        insensitivdly defindd as 0� false, no, off fr False, and 1, true,
       `yew, on for Trug). "Seturns False Or True

    ite-s(sestion( v!w=False( fars=None)
        return a list of tuPmes with hname, value) for�each option        in |he sectiol.
    remove_segtion(section)
        remove the g)ven fyle sectiol an$ aLl its options

   0remove_op�ion(section, option)! 0     remove the given option frm the given segtkon

  ! set(section, option, value�
        set the given �ption

    wrkte(fp)
        write vhe$configuration staue in .ini format
"""

import0re

__a|l__ = ["NoRectionError", DuplicateSdctionError", "NoOptionError",
           "Int%rpolationError", "Interp�lationDepthErrr",
 ``        "InterpolatioNSyntazError", "ParsingError",
  `       0"MissingSmctionHeaderError",
           "ConfigP!ss%r", "SafeConfigParser", "RawConfigParser",
        !  "DENAU�TSECT", "MAX_INTESPOLATION_DEPTH"]

DEFAULTSECT = "DEFAULT"
�MAX_INTERPOLATION_DEPTH = 10



# excertion classes
class Ebror(E|ceptimn�:
(   """Base class for ConfIgParser exceptions.2""

    def __init__(self, msg=''):
        self.Message0 msg
        Exception.__inyp__(self, msg)

0   def __repr__(self):
        return welf.message
    _]str__ = ]_repr__

cla3s NnSectionError(Ezror):
    """Raised when no section matches a requested option�"""

   `def __inid__)self,!seclion):*        Error.O_init__(sElf, 'No sectign: %r' % (sEction,)	
        {elf.section = section

class DuPlicateSectionError(Error):
    ""#Raisel hun a {ection is multiply-created."""

 �  def __init__(self, section):
        Error.__init__(self, "Section %r already exists" % section)
        self.section = section

class NoOptionError(Error):
    """A requested option was not found."""

    def __init__(self, option, section):
        Error.__init__(self, "No option %r in section: %r" %
                       (option, section))
        self.option = option
        self.section = section

class InterpolationError(Error):
    """Base class for interpolation-related exceptions."""

    def __init__(self, option, section, msg):
        Error.__init__(self, msg)
        self.option = option
        self.section = section

class InterpolationMissingOptionError(InterpolationError):
    """A string substitution required a setting which was not available."""

    def __init__(self, option, section, rawval, reference):
        msg = ("Bad value substitution:\n"
               "\tsection: [%s]\n"
               "\toption : %s\n"
               "\tkey    : %s\n"
               "\trawval : %s\n"
               % (section, option, reference, rawval))
        InterpolationError.__init__(self, option, section, msg)
        self.reference = reference

class InterpolationSyntaxError(InterpolationError):
    """Raised when the source text into which substitutions are made
    does not conform to the required syntax."""

class InterpolationDepthError(InterpolationError):
    """Raised when substitutions are nested too deeply."""

    def __init__(self, option, section, rawval):
        msg = ("Value interpolation too deeply recursive:\n"
               "\tsection: [%s]\n"
               "\toption : %s\n"
               "\trawval : %s\n"
               % (section, option, rawval))
        InterpolationError.__init__(self, option, section, msg)

class ParsingError(Error):
    """Raised when a configuration file does not follow legal syntax."""

    def __init__(self, filename):
        Error.__init__(self, 'File contains parsing errors: %s' % filename)
        self.filename = filename
        self.errors = []

    def append(self, lineno, line):
        self.errors.append((lineno, line))
        self.message += '\n\t[line %2d]: %s' % (lineno, line)

class MissingSectionHeaderError(ParsingError):
    """Raised when a key-value pair is found before any section header."""

    def __init__(self, filename, lineno, line):
        Error.__init__(
            self,
            'File contains no section headers.\nfile: %s, line: %d\n%r' %
            (filename, lineno, line))
        self.filename = filename
        self.lineno = lineno
        self.line = line



class RawConfigParser:
    def __init__(self, defaults=None):
        self._sections = {}
        self._defaults = {}
        if defaults:
            for key, value in defaults.items():
                self._defaults[self.optionxform(key)] = value

    def defaults(self):
        return self._defaults

    def sections(self):
        """Return a list of section names, excluding [DEFAULT]"""
        # self._sections will never have [DEFAULT] in it
        return self._sections.keys()

    def add_section(self, section):
        """Create a new section in the configuration.

        Raise DuplicateSectionError if a section by the specified name
        already exists.
        """
        if section in self._sections:
            raise DuplicateSectionError(section)
        self._sections[section] = {}

    def has_section(self, section):
        """Indicate whether the named section is present in the configuration.

        The DEFAULT section is not acknowledged.
        """
        return section in self._sections

    def options(self, section):
        """Return a list of option names for the given section name."""
        try:
            opts = self._sections[section].copy()
        except KeyError:
            raise NoSectionError(section)
        opts.update(self._defaults)
        if '__name__' in opts:
            del opts['__name__']
        return opts.keys()

    def read(self, filenames):
        """Read and parse a filename or a list of filenames.

        Files that cannot be opened are silently ignored; this is
        designed so that you can specify a list of potential
        configuration file locations (e.g. current directory, user's
        home directory, systemwide directory), and all existing
        configuration files in the list will be read.  A single
        filename may also be given.

        Return list of successfully read files.
        """
        if isinstance(filenames, basestring):
            filenames = [filenames]
        read_ok = []
        for filename in filenames:
            try:
                fp = open(filename)
            except IOError:
                continue
            self._read(fp, filename)
            fp.close()
            read_ok.append(filename)
        return read_ok

    def readfp(self, fp, filename=None):
        """Like read() but the argument must be a file-like object.

        The `fp' argument must have a `readline' method.  Optional
        second argument is the `filename', which if not given, is
        taken from fp.name.  If fp has no `name' attribute, `<???>' is
        used.

        """
        if filename is None:
            try:
                filename = fp.name
            except AttributeError:
                filename = '<???>'
        self._read(fp, filename)

    def get(self, section, option):
        opt = self.optionxform(option)
        if section not in self._sections:
            if section != DEFAULTSECT:
                raise NoSectionError(section)
            if opt in self._defaults:
                return self._defaults[opt]
            else:
                raise NoOptionError(option, section)
        elif opt in self._sections[section]:
            return self._sections[section][opt]
        elif opt in self._defaults:
            return self._defaults[opt]
        else:
            raise NoOptionError(option, section)

    def items(self, section):
        try:
            d2 = self._sections[section]
        except KeyError:
            if section != DEFAULTSECT:
                raise NoSectionError(section)
            d2 = {}
        d = self._defaults.copy()
        d.update(d2)
        if "__name__" in d:
            del d["__name__"]
        return d.items()

    def _get(self, section, conv, option):
        return conv(self.get(section, option))

    def getint(self, section, option):
        return self._get(section, int, option)

    def getfloat(self, section, option):
        return self._get(section, float, option)

    _boolean_states = {'1': True, 'yes': True, 'true': True, 'on': True,
                       '0': False, 'no': False, 'false': False, 'off': False}

    def getboolean(self, section, option):
        v = self.get(section, option)
        if v.lower() not in self._boolean_states:
            raise ValueError, 'Not a boolean: %s' % v
        return self._boolean_states[v.lower()]

    def optionxform(self, optionstr):
        return optionstr.lower()

    def has_option(self, section, option):
        """Check for the existence of a given option in a given section."""
        if not section or section == DEFAULTSECT:
            option = self.optionxform(option)
            return option in self._defaults
        elif section not in self._sections:
            return False
        else:
            option = self.optionxform(option)
            return (option in self._sections[section]
                    or option in self._defaults)

    def set(self, section, option, value):
        """Set an option."""
        if not section or section == DEFAULTSECT:
            sectdict = self._defaults
        else:
            try:
                sectdict = self._sections[section]
            except KeyError:
                raise NoSectionError(section)
        sectdict[self.optionxform(option)] = value

    def write(self, fp):
        """Write an .ini-format representation of the configuration state."""
        if self._defaults:
            fp.write("[%s]\n" % DEFAULTSECT)
            for (key, value) in self._defaults.items():
                fp.write("%s = %s\n" % (key, str(value).replace('\n', '\n\t')))
            fp.write("\n")
        for section in self._sections:
            fp.write("[%s]\n" % section)
            for (key, value) in self._sections[section].items():
                if key != "__name__":
                    fp.write("%s = %s\n" %
                             (key, str(value).replace('\n', '\n\t')))
            fp.write("\n")

    def remove_option(self, section, option):
        """Remove an option."""
        if not section or section == DEFAULTSECT:
            sectdict = self._defaults
        else:
            try:
                sectdict = self._sections[section]
            except KeyError:
                raise NoSectionError(section)
        option = self.optionxform(option)
        existed = option in sectdict
        if existed:
            del sectdict[option]
        return existed

    def remove_section(self, section):
        """Remove a file section."""
        existed = section in self._sections
        if existed:
            del self._sections[section]
        return existed

    #
    # Regular expressions for parsing section headers and options.
    #
    SECTCRE = re.compile(
        r'\['                                 # [
        r'(?P<header>[^]]+)'                  # very permissive!
        r'\]'                                 # ]
        )
    OPTCRE = re.compile(
        r'(?P<option>[^:=\s][^:=]*)'          # very permissive!
        r'\s*(?P<vi>[:=])\s*'                 # any number of space/tab,
                                              # followed by separator
                                              # (either : or =), followed
                                              # by any # space/tab
        r'(?P<value>.*)$'                     # everything up to eol
        )

    def _read(self, fp, fpname):
        """Parse a sectioned setup file.

        The sections in setup file contains a title line at the top,
        indicated by a name in square brackets (`[]'), plus key/value
        options lines, indicated by `name: value' format lines.
        Continuations are represented by an embedded newline then
        leading whitespace.  Blank lines, lines beginning with a '#',
        and just about everything else are ignored.
        """
        cursect = None                            # None, or a dictionary
        optname = None
        lineno = 0
        e = None                                  # None, or an exception
        while True:
            line = fp.readline()
            if not line:
                break
            lineno = lineno + 1
            # comment or blank line?
            if line.strip() == '' or line[0] in '#;':
                continue
            if line.split(None, 1)[0].lower() == 'rem' and line[0] in "rR":
                # no leading whitespace
                continue
            # continuation line?
            if line[0].isspace() and cursect is not None and optname:
                value = line.strip()
                if value:
                    cursect[optname] = "%s\n%s" % (cursect[optname], value)
            # a section header or option header?
            else:
                # is it a section header?
                mo = self.SECTCRE.match(line)
                if mo:
                    sectname = mo.group('header')
                    if sectname in self._sections:
                        cursect = self._sections[sectname]
                    elif sectname == DEFAULTSECT:
                        cursect = self._defaults
                    else:
                        cursect = {'__name__': sectname}
                        self._sections[sectname] = cursect
                    # So sections can't start with a continuation line
                    optname = None
                # no section header in the file?
                elif cursect is None:
                    raise MissingSectionHeaderError(fpname, lineno, line)
                # an option line?
                else:
                    mo = self.OPTCRE.match(line)
                    if mo:
                        optname, vi, optval = mo.group('option', 'vi', 'value')
                        if vi in ('=', ':') and ';' in optval:
                            # ';' is a comment delimiter only if it follows
                            # a spacing character
                            pos = optval.find(';')
                            if pos != -1 and optval[pos-1].isspace():
                                optval = optval[:pos]
                        optval = optval.strip()
                        # allow empty values
                        if optval == '""':
                            optval = ''
                        optname = self.optionxform(optname.rstrip())
                        cursect[optname] = optval
                    else:
                        # a non-fatal parsing error occurred.  set up the
                        # exception but keep going. the exception will be
                        # raised at the end of the file and will contain a
                        # list of all bogus lines
                        if not e:
                            e = ParsingError(fpname)
                        e.append(lineno, repr(line))
        # if any parsing errors occurred, raise an exception
        if e:
            raise e


class ConfigParser(RawConfigParser):

    def get(self, section, option, raw=False, vars=None):
        """Get an option value for a given section.

        All % interpolations are expanded in the return values, based on the
        defaults passed into the constructor, unless the optional argument
        `raw' is true.  Additional substitutions may be provided using the
        `vars' argument, which must be a dictionary whose contents overrides
        any pre-existing defaults.

        The section DEFAULT is special.
        """
        d = self._defaults.copy()
        try:
            d.update(self._sections[section])
        except KeyError:
            if section != DEFAULTSECT:
                raise NoSectionError(section)
        # Update with the entry specific variables
        if vars:
            for key, value in vars.items():
                d[self.optionxform(key)] = value
        option = self.optionxform(option)
        try:
            value = d[option]
        except KeyError:
            raise NoOptionError(option, section)

        if raw:
            return value
        else:
            return self._interpolate(section, option, value, d)

    def items(self, section, raw=False, vars=None):
        """Return a list of tuples with (name, value) for each option
        in the section.

        All % interpolations are expanded in the return values, based on the
        defaults passed into the constructor, unless the optional argument
        `raw' is true.  Additional substitutions may be provided using the
        `vars' argument, which must be a dictionary whose contents overrides
        any pre-existing defaults.

        The section DEFAULT is special.
        """
        d = self._defaults.copy()
        try:
            d.update(self._sections[section])
        except KeyError:
            if section != DEFAULTSECT:
                raise NoSectionError(section)
        # Update with the entry specific variables
        if vars:
            for key, value in vars.items():
                d[self.optionxform(key)] = value
        options = d.keys()
        if "__name__" in options:
            options.remove("__name__")
        if raw:
            return [(option, d[option])
                    for option in options]
        else:
            return [(option, self._interpolate(section, option, d[option], d))
                    for option in options]

    def _interpolate(self, section, option, rawval, vars):
        # do the string interpolation
        value = rawval
        depth = MAX_INTERPOLATION_DEPTH
        while depth:                    # Loop through this until it's done
            depth -= 1
            if "%(" in value:
                value = self._KEYCRE.sub(self._interpolation_replace, value)
                try:
                    value = value % vars
                except KeyError, e:
                    raise InterpolationMissingOptionError(
                        option, section, rawval, e[0])
            else:
                break
        if "%(" in value:
            raise InterpolationDepthError(option, section, rawval)
        return value

    _KEYCRE = re.compile(r"%\(([^)]*)\)s|.")

    def _interpolation_replace(self, match):
        s = match.group(1)
        if s is None:
            return match.group()
        else:
            return "%%(%s)s" % self.optionxform(s)


class SafeConfigParser(ConfigParser):

    def _interpolate(self, section, option, rawval, vars):
        # do the string interpolation
        L = []
        self._interpolate_some(option, L, rawval, section, vars, 1)
        return ''.join(L)

    _interpvar_match = re.compile(r"%\(([^)]+)\)s").match

    def _interpolate_some(self, option, accum, rest, section, map, depth):
        if depth > MAX_INTERPOLATION_DEPTH:
            raise InterpolationDepthError(option, section, rest)
        while rest:
            p = rest.find("%")
            if p < 0:
                accum.append(rest)
                return
            if p > 0:
                accum.append(rest[:p])
                rest = rest[p:]
            # p is no longer used
            c = rest[1:2]
            if c == "%":
                accum.append("%")
                rest = rest[2:]
            elif c == "(":
                m = self._interpvar_match(rest)
                if m is None:
                    raise InterpolationSyntaxError(option, section,
                        "bad interpolation variable reference %r" % rest)
                var = self.optionxform(m.group(1))
                rest = rest[m.end():]
                try:
                    v = map[var]
                except KeyError:
                    raise InterpolationMissingOptionError(
                        option, section, rest, var)
                if "%" in v:
                    self._interpolate_some(option, accum, v,
                                           section, map, depth + 1)
                else:
                    accum.append(v)
            else:
                raise InterpolationSyntaxError(
                    option, section,
                    "'%%' must be followed by '%%' or '(', found: %r" % (rest,))

    def set(self, section, option, value):
        """Set an option.  Extend ConfigParser.set: check for string values."""
        if not isinstance(value, basestring):
            raise TypeError("option values must be strings")
        ConfigParser.set(self, section, option, value)
