/*! \file worldsteptask.h
    

    A world step task can perform world step, basing on rendering intervals
 */
#pragma once
#include "world.h"
#include "../pipeline/pipelineprocess.h"
#include "../renderer.h"

namespace sad
{

namespace p2d
{

/*! A world step task can perform world step, basing on rendering intervals
 */
class WorldStepTask: public sad::pipeline::AbstractProcess
{
public:
    /*! Constructs new step task. Renderer is referenced weakly here, so it should 
        not be relied, so task won't be dead, when it's called
        \param[in] w world
        \param[in] r renderer
     */
    WorldStepTask(sad::p2d::World* w, sad::Renderer* r = sad::Renderer::ref());
    /*! Sets a world
        \param[in] world a world
     */
    void setWorld(sad::p2d::World* world);
    /*! Decreases reference count for world 
     */
    ~WorldStepTask();
protected:
    /*! A renderer, which will supply a task with data about FPS
     */
    sad::Renderer* m_renderer;
    /*! A world, that will be calculated, when task is executed
     */
    sad::p2d::World* m_world;
    /*! Invokes a delegate inside of process
     */ 
    virtual void _process();
};

}

}
