#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member, DEFAULT_VALUE, VALIDATE_ENUM


class Enum(Member):
    """ A member where the value can be one in a group of items.

    """
    __slots__ = ()

    def __init__(self, *items):
        """ Initialize an Enum.

        Parameters
        ----------
        *items
            The allowed values which can be assigned to the enum.

        """
        if len(items) == 0:
            raise ValueError('an Enum requires at least 1 item')
        self.set_default_kind(DEFAULT_VALUE, items[0])
        self.set_validate_kind(VALIDATE_ENUM, items)

    @property
    def items(self):
        """ A readonly property which returns the items in the enum.

        """
        return self.validate_kind[1]

    def extend(self, *items):
        """ Create a clone of the Enum with additional items.

        Parameters
        ----------
        *items
            Additional items to include in the Enum.

        Returns
        -------
        result : Enum
            A new enum object which contains all of the original items
            plus the new items.

        """
        olditems = self.items
        newitems = olditems + items
        clone = self.clone()
        clone.set_validate_kind(VALIDATE_ENUM, newitems)
        return clone

    def restrict(self, *items):
        """ Create a clone of the Enum with some items removed.

        Parameters
        ----------
        *items
            The items to remove remove from the new enum.

        Returns
        -------
        result : Enum
            A new enum object which contains all of the original items
            but with the given items removed.

        """
        olditems = self.items
        newitems = tuple(i for i in olditems if i not in items)
        if len(newitems) == 0:
            raise ValueError('an Enum requires at least 1 item')
        clone = self.clone()
        clone.set_default_kind(DEFAULT_VALUE, newitems[0])
        clone.set_validate_kind(VALIDATE_ENUM, newitems)
        return clone

    def __call__(self, item):
        """ Create a clone of the Enum item with a new default.

        Parameters
        ----------
        item : object
            The item to use as the Enum default. The item must be one
            of the valid enum items.

        """
        olditems = self.items
        if item not in olditems:
            raise TypeError('invalid enum value')
        clone = self.clone()
        clone.set_default_kind(DEFAULT_VALUE, item)
        return clone

