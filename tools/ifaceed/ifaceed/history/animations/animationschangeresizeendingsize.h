/*! \file animationschangeresizeendingsize.h
    

    Describe a command, which changes ending size of resize animation
 */
#pragma once
#include "animationschangeproperty.h"

namespace history
{

namespace animations
{
/*! A command,which changes ending size of resize animation
 */
class ChangeResizeEndingSize: public history::animations::ChangeProperty<sad::Point2D>
{
public:
     /*! Constructs new command for animation
        \param[in] d an animation
        \param[in] oldvalue old value
        \param[in] newvalue new value
      */
    ChangeResizeEndingSize(sad::animations::Animation* d, const sad::Point2D& oldvalue, const sad::Point2D& newvalue);
    /*! Erases link to an animation
      */
    virtual ~ChangeResizeEndingSize();
protected:
    /*!
     * Updates current text in field with object name
     * \param[in] e editor
     * \param[in] value a value
     */
    virtual void updateUI(core::Editor* e, const sad::Point2D& value);
};

}

}
