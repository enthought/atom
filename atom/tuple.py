#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
class Tuple(Value):
    """ A value of type `tuple`.

    """
    __slots__ = ()

    def __init__(self, default=(), factory=None):
        super(Tuple, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_TUPLE, None)