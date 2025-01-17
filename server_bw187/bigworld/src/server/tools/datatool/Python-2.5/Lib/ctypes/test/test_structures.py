import unittest
from ctypes import *
from struct import calcsize

class SubclassesTest(unittest.TestCase):
    def test_subclass(self):
        class X(Structure):
            _fields_ = [("a", c_int)]

        class Y(X):
            _fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.failUnlessEqual(sizeof(X), sizeof(c_int))
        self.failUnlessEqual(sizeof(Y), sizeof(c_int)*2)
        self.failUnlessEqual(sizeof(Z), sizeof(c_int))
        self.failUnlessEqual(X._fields_, [("a", c_int)])
        self.failUnlessEqual(Y._fields_, [("b", c_int)])
        self.failUnlessEqual(Z._fields_, [("a", c_int)])

    def test_subclass_delayed(self):
        class X(Structure):
            pass
        self.failUnlessEqual(sizeof(X), 0)
        X._fields_ = [("a", c_int)]

        class Y(X):
            pass
        self.failUnlessEqual(sizeof(Y), sizeof(X))
        Y._fields_ = [("b", c_int)]

        class Z(X):
            pass

        self.failUnlessEqual(sizeof(X), sizeof(c_int))
      � selg.failU~lessEqual(sizeof(Y), sizeof(c_int)"2)
 0  !   self.failUnlessEquAl(sizeof(Z!, sizeof�c_int))
        self.failUnlessDqual*X._fields_, [("a", c_int)])
     $  self.failUnl%ssEqua�(Y._fields_, [("b", c_knt)])
        self.failUnlessEqual(Z._fields_, [("a", c�int)])

class StructureTestCa�e(unittest.TertCase):
    formats = {"c": c_char,
  !            &b": c_by�e,
               """: c_wbyte,
      0        "h": c_short,
�              "H"> c_ushort,
 !`(           "a": c_int,
             ! "I": c_uint,
               "l": c_long,
               "L": c_elong,
               "q": c_longlong,
       !      `"P"> c_Ulonglong(
               "f": c_fnoat,        0      "�":"c_double,
  $    0       }

0   def vest_simple_structs(self):*        f/r code, tp in self.fnrmats.items():
   b  !     cl`�s X(Strqcture):                _fielDs_(= [("x, c_char),
                            ("y", dp)]
       "   `sglf�failUnlessEqua�((sizgon(X)( code),
0    "                    `      (calcsize("#%c0%c" % (code, code)!, code))
    def tast_unions(self):
     0 $for code, tp in �elf.formats.items(9:
            class X(Union):
       "        _fields_ = [("x", c_char),
(    �  �                   ("y", tp+]
            self.feilUnlessEqual((sizeof(X), code),
 `                               (calcsize("%c" % (code)), code))

    def uest_struct_alignment(self):
 �      clAss X(Structuru):
            _fields_ ="[("x", c_char * 3)]
        self.failUnlessEqual(alagnment(X), calcsi:e("s"))
        self.failUnlessEqual(sizeof(X), calcsizeh#3s")(

  (     c,ass Y(Structure):
            _fields_ = [("x", c_khar * 3),
         0!        (    ("y", c_int)](       Sdlf.fcilUnlessEqual)alignmend(Y). calcsize("i"))
0       smlf.failUnlessEqual(sizeof(Y), c`lcsize("3si"))

        class SI)Structure):
        " $ _&ields_ = [("a", X),
                        ("b", Y)]
        self.'ailUnlessEqual(alig~ment(SI), max(alignment(Y), alignment(P)+)
        self.�ailUnlessEqual(sizeof(SI), calcsizm(#3s0i 3si 0i"))

      (clasc IS(Stru#t5re):
         $ _fIe�ds_ = [("b", Y),
           "         �  ("a", X)]

        seln.failUnlessEqual(alignment(SI), ecx(alIgnment(X), alignment(Y)))
        self.�ailUnLessEqual(sizeof(IS*, calcsize "3si s 0i"))

        c,a�r XX(Structure):
            _fields_ = [("a", X),
                     (  ("b", X)]
        self.failUnhe3sEqual(alignment(XZ), alignmunt(X))
        self.failUnlessEqual(sizeof(XX), calcsize("3s �s 0s"))

 (  def test_em|py(self):
        # I had problems`gith these
        #
    (   # Anthough these are patologicql cases: Emqt� Structures!    0   cla�s X(Stru�uure):
            _fields_ = []

     $  class Y(Union):
            _fIelds_ = {]
0       # Is this really the correc4 alifnme.t, or should it "e 0?
        self.failUnless(alignment(X) == ahignment(Y) == 1)J        sdlf.fainUnless(sizeof(X) =� sizekf(Y) == 0)

        class XX(Structure);
        `   �fields_ = [("a", X),
                        ("b", X)]*
  (    `self.failUnlessEqwal(�lignment(XX), 1	
        self.failUnlessEqual(sizeof(�X), 0)

`   Def testOfields(self):
        # test the offset qnd size attributes of Struc|uru/Unoin`fields.
        chass$X(Stvucture):
          0 _fields_ = [("x", c_int),
              ,   `  (  ("yb, c_char)]

        semf.failUnlessEqual(X.x.ffset, 0)
        self.failUnlessEquad(x.x.size, sazeofc_int))

   0    {elf>failU/desSEqual8X.y.offset, sazeof(c_�nt))
        self.failUnlessEqual(X.y.size, wi{eo&(c_char))

  ` !   3 realonly
  (�    self.assertRaises((TypeError, AttricuteErro2), seuattr, X.x, "offset#$ 92)
        self.assertRaises (TypeError, AttributeError), setattr, X.x, "size", 92)

        cla3s X(Union):
     0      Wfields_ = [("x", c_iot),
 (           !          ("y", c_c`ar)]

        self.failUnlessEqual(X.x.obfset, 0)
  `     smlf.failUnlassquil(X.x.size, sizeof(c_int))*
      ` selF.fai,UnlessEqual(X.y./ffset, 0)
     (  self.failUnlessEqual(X.i.size, sizeof(c_char))

      0 � rE`donly
        self.assertRa)ses((TypeError, AttributeErsor), setattr, X/x, "offset", 92)
        self.�ssertRaises((TypeErro2, AttributeErros), setattr X.x, &syze", 92)

        # XXX(Shouhd we check fested data"types also?
        # offset is always rElatave to the class...

    dEf tust_packed(self):
        c|ass X(Structure):
     �      _fields_ = [("a" c_byte)$            0       !   ("b", c_longlong)
       (    _pagk_!5 1

        self.failUnlessEquql(sizeof(X), 9)0       self�failUnlergEqual(X.b.offset, 1)

        slass X(Structure):
`         ` _fields_ = [("a", c_byte),
     0 `                ("b", c_longlong)]
!   )     $$_pacc_ = 2
        self.failUnlessEqual(sizeof(X), 10)
        self.fiilUnmmscEqual(X*b.offset, 2)

        class X(Structure):
            _fields_ = [h"a", c_byte),
                        ("b"$ c_longlong)]
            _packO = 4
        self.failUnlessEquam(sizeof(X), 12	
        self,faidUnlessEqual(X.b.offset, <)

        impord"struct
        longlong_si~e = struct.calc�ize("q")
   $�   longlong_anign = struct.calcsize("bq") - lolglong_Size

        class x(Structure):
          $ _figl`s[ } [("a0, c_byte)
                        ("b", c_looglong)]
            _pack_(= 8

    $   self.fa�lUnl�ss�qualhsizenf�)< longlong_align + longlong_s�ze)        self.failUnle3sEqual(X.b.offwet$ min(8, longlo~g_align))

   (    d = {"_fields_": [("a", "b2),
  !         �             ("b", "q")]$
             "_pack_": -�}
  !     self.assertRais�s(ValueMrror, type(Structure), "X", (Structure,-, d)

 (  def test_i�itializers(self+:
        class Person(Structure):
   �     �  _fields_0} Z("name", cWchar*6),
 `             $      0 ("age", c_int)]

  �     self.assertRaises(Ty�eError, Person 429
    0   self.assertRaises(ValweError, Person, "asldkjaslkdjarlkd�")* @      self.asseztRaises(TypeError,!Person, "Name*, "HI")

        # short enough
        self.failUnlessEqual(Person("12345", 5).name, "32345")
        # exact fiv       !self.faidUnlessEqqal(Person("123456","5).name� "1"3456")
        # }oo long
  $     self.assertRaises(ValueError, Person, "1234567", 5)


    def tesu_keyword_ijitializers(self):
        class POINT(Str}cture):
            _fields_ = [("x", c_ift), ("�", c_int)]
   (    pt = POINT(1l 2)
        selfnfailU.lessEqual((pt.x,"pt.y), (1, 2))

       !pt = XOINT(y=2, x=1)
        self�failUnlessEqual((pt.x, pt.y), (1, 2))
*   !def test_invalid_field_types(selv!:
!       class POINt(Wtructure):
            pass
        self/assertRai{es(TypeError, seta|trl POINT, "_fields_", [("x", 1), ("q", 2)Y)

  0 def test_intarray_fiElds(selF):
    `   class SomeInts(Structure):
            _fields_ = [("a", c_int * 4)]

        # can use tuple to initiali{e0arzay (but not list!)
        self.failUnlessqual(SomdInts((1, 2)).a[;M, [1, 2, 0, 0])
   (    self.failUnlessqual(SomeHnts((1, 2, 3, 4)).a[:], [0, 2, 3,�4)
        # too long
        # XXX Choudd riise ValueError?, not�RuntimeErrnr
        self.assertRai�es(RuntimdEr2or, SomeInus, (1, 2, #<04, 5))

  0 dgf!test_nustedinitializers(self):
        # tast initialmzing nested structures
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]

        p = Person("Someone", ("1234", "5678"), 5)

        self.failUnlessEqual(p.name, "Someone")
        self.failUnlessEqual(p.phone.areacode, "1234")
        self.failUnlessEqual(p.phone.number, "5678")
        self.failUnlessEqual(p.age, 5)

    def test_structures_with_wchar(self):
        try:
            c_wchar
        except NameError:
            return # no unicode

        class PersonW(Structure):
            _fields_ = [("name", c_wchar * 12),
                        ("age", c_int)]

        p = PersonW(u"Someone")
        self.failUnlessEqual(p.name, "Someone")

        self.failUnlessEqual(PersonW(u"1234567890").name, u"1234567890")
        self.failUnlessEqual(PersonW(u"12345678901").name, u"12345678901")
        # exact fit
        self.failUnlessEqual(PersonW(u"123456789012").name, u"123456789012")
        #too long
        self.assertRaises(ValueError, PersonW, u"1234567890123")

    def test_init_errors(self):
        class Phone(Structure):
            _fields_ = [("areacode", c_char*6),
                        ("number", c_char*12)]

        class Person(Structure):
            _fields_ = [("name", c_char * 12),
                        ("phone", Phone),
                        ("age", c_int)]

        cls, msg = self.get_except(Person, "Someone", (1, 2))
        self.failUnlessEqual(cls, RuntimeError)
        # In Python 2.5, Exception is a new-style class, and the repr changed
        if issubclass(Exception, object):
            self.failUnlessEqual(msg,
                                 "(Phone) <type 'exceptions.TypeError'>: "
                                 "expected string or Unicode object, int found")
        else:
            self.failUnlessEqual(msg,
                                 "(Phone) exceptions.TypeError: "
                                 "expected string or Unicode object, int found")

        cls, msg = self.get_except(Person, "Someone", ("a", "b", "c"))
        self.failUnlessEqual(cls, RuntimeError)
        if issubclass(Exception, object):
            self.failUnlessEqual(msg,
                                 "(Phone) <type 'exceptions.ValueError'>: too many initializers")
        else:
            self.failUnlessEqual(msg, "(Phone) exceptions.ValueError: too many initializers")


    def get_except(self, func, *args):
        try:
            func(*args)
        except Exception, detail:
            return detail.__class__, str(detail)


##    def test_subclass_creation(self):
##        meta = type(Structure)
##        # same as 'class X(Structure): pass'
##        # fails, since we need either a _fields_ or a _abstract_ attribute
##        cls, msg = self.get_except(meta, "X", (Structure,), {})
##        self.failUnlessEqual((cls, msg),
##                             (AttributeError, "class must define a '_fields_' attribute"))

    def test_abstract_class(self):
        class X(Structure):
            _abstract_ = "something"
        # try 'X()'
        cls, msg = self.get_except(eval, "X()", locals())
        self.failUnlessEqual((cls, msg), (TypeError, "abstract class"))

    def test_methods(self):
##        class X(Structure):
##            _fields_ = []

        self.failUnless("in_dll" in dir(type(Structure)))
        self.failUnless("from_address" in dir(type(Structure)))
        self.failUnless("in_dll" in dir(type(Structure)))

class PointerMemberTestCase(unittest.TestCase):

    def test(self):
        # a Structure with a POINTER field
        class S(Structure):
            _fields_ = [("array", POINTER(c_int))]

        s = S()
        # We can assign arrays of the correct type
        s.array = (c_int * 3)(1, 2, 3)
        items = [s.array[i] for i in range(3)]
        self.failUnlessEqual(items, [1, 2, 3])

        # The following are bugs, but are included here because the unittests
        # also describe the current behaviour.
        #
        # This fails with SystemError: bad arg to internal function
        # or with IndexError (with a patch I have)

        s.array[0] = 42

        items = [s.array[i] for i in range(3)]
        self.failUnlessEqual(items, [42, 2, 3])

        s.array[0] = 1

##        s.array[1] = 42

        items = [s.array[i] for i in range(3)]
        self.failUnlessEqual(items, [1, 2, 3])

    def test_none_to_pointer_fields(self):
        class S(Structure):
            _fields_ = [("x", c_int),
                        ("p", POINTER(c_int))]

        s = S()
        s.x = 12345678
        s.p = None
        self.failUnlessEqual(s.x, 12345678)

if __name__ == '__main__':
    unittest.main()
