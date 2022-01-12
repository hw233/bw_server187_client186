/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"
#include "link_property.hpp"


GENPROPERTY_VIEW_FACTORY(LinkProperty)


/**
 *  LinkProperty constructor.
 *
 *  @param name         The name of the property.
 *  @param linkProxy    A proxy object used to test linkages, to create
 *                      linkages etc.
 *  @param matrix       The position/orientation of the item being linked.
 */
LinkProperty::LinkProperty
(
    std::string         const &name,
    LinkProxyPtr        linkProxy,
    MatrixProxyPtr      matrix    
)
:
GeneralProperty(name),
matrix_(matrix),
linkProxy_(linkProxy)
{
    GENPROPERTY_MAKE_VIEWS()
}


/**
 *  Not implemented.
 */
/*virtual*/ PyObject * EDCALL LinkProperty::pyGet()
{
    return NULL;
}


/**
 *  Not implemented.
 */
/*virtual*/ int EDCALL LinkProperty::pySet
(
    PyObject        *value, 
    bool            transient   /*= false*/
)
{
    return 0;
}


/**
 *  Get the position/orientation proxy for the item being linked.
 *
 *  @returns        The position/orientation proxy for the item being linked.
 */
MatrixProxyPtr LinkProperty::matrix() const 
{ 
    return matrix_; 
}


/**
 *  Get the link proxy for the item being linked.
 *
 *  @returns        The link proxy for the item being linked.
 */
LinkProxyPtr LinkProperty::link() const
{
    return linkProxy_;
}
