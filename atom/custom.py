#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member, null, USER_DEFAULT, USER_VALIDATE


class CustomMember(Member):
    """ A member which handles all defaults and validation from Python.

    This class is intended to be used as a base class for third party
    libraries which need to implement their own custom member behavior.
    Subclasses should reimplement the 'default' and 'validate' methods
    as needed.

    """
    __slots__ = ()

    def __init__(self):
        self.default_kind = (USER_DEFAULT, None)
        self.validate_kind = (USER_VALIDATE, None)

    def default(self, owner):
        """ Reimplement in a subclass to compute the default value.

        """
        return null

    def validate(self, owner, old, new):
        """ Reimplement in a subclass to perform validation.

        """
        return new

