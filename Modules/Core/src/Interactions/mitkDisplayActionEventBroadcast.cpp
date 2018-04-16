/*===================================================================

 The Medical Imaging Interaction Toolkit (MITK)

 Copyright (c) German Cancer Research Center,
 Division of Medical Image Computing.
 All rights reserved.

 This software is distributed WITHOUT ANY WARRANTY; without
 even the implied warranty of MERCHANTABILITY or FITNESS FOR
 A PARTICULAR PURPOSE.

 See LICENSE.txt or http://www.mitk.org for details.

 ===================================================================*/

#include "mitkDisplayActionEventBroadcast.h"

 // us
#include "usGetModuleContext.h"
#include "usModuleContext.h"

// mitk core module
#include "mitkDisplayActionEvents.h"
#include "mitkInteractionPositionEvent.h"
#include "mitkLine.h"

mitk::DisplayActionEventBroadcast::DisplayActionEventBroadcast()
  : m_AlwaysReact(false)
  , m_AutoRepeat(false)
  , m_IndexToSliceModifier(4)
  , m_InvertScrollDirection(false)
  , m_InvertZoomDirection(false)
  , m_ZoomFactor(2)
  , m_InvertMoveDirection(false)
  , m_InvertLevelWindowDirection(false)
  , m_LinkPlanes(true)
{
  m_StartCoordinateInMM.Fill(0);
  m_LastDisplayCoordinate.Fill(0);
  m_LastCoordinateInMM.Fill(0);
  m_CurrentDisplayCoordinate.Fill(0);

  // register the broadcast class as interaction event observer via micro services
  us::ServiceProperties props;
  props["name"] = std::string("DisplayActionEventBroadcast");
  m_ServiceRegistration = us::GetModuleContext()->RegisterService<mitk::InteractionEventObserver>(this, props);
}

mitk::DisplayActionEventBroadcast::~DisplayActionEventBroadcast()
{
  m_ServiceRegistration.Unregister();
}

void mitk::DisplayActionEventBroadcast::Notify(InteractionEvent* interactionEvent, bool isHandled)
{
  // the event is passed to the state machine interface to be handled
  if (!isHandled || m_AlwaysReact)
  {
    HandleEvent(interactionEvent, nullptr);
  }
}

void mitk::DisplayActionEventBroadcast::ConnectActionsAndFunctions()
{
  CONNECT_CONDITION("check_position_event", CheckPositionEvent);
  CONNECT_CONDITION("check_can_rotate", CheckRotationPossible);
  CONNECT_CONDITION("check_can_swivel", CheckSwivelPossible);

  CONNECT_FUNCTION("init", Init);
  CONNECT_FUNCTION("move", Move);
  CONNECT_FUNCTION("zoom", Zoom);
  CONNECT_FUNCTION("scroll", Scroll);
  CONNECT_FUNCTION("ScrollOneUp", ScrollOneUp);
  CONNECT_FUNCTION("ScrollOneDown", ScrollOneDown);
  CONNECT_FUNCTION("levelWindow", AdjustLevelWindow);
  CONNECT_FUNCTION("setCrosshair", SetCrosshair);

  CONNECT_FUNCTION("updateStatusbar", UpdateStatusbar)

  CONNECT_FUNCTION("startRotation", StartRotation);
  CONNECT_FUNCTION("endRotation", EndRotation);
  CONNECT_FUNCTION("rotate", Rotate);

  CONNECT_FUNCTION("swivel", Swivel);
}

void mitk::DisplayActionEventBroadcast::ConfigurationChanged()
{
  mitk::PropertyList::Pointer properties = GetAttributes();

  // allwaysReact
  std::string strAlwaysReact = "";
  if (properties->GetStringProperty("alwaysReact", strAlwaysReact))
  {
    if (strAlwaysReact == "true")
    {
      m_AlwaysReact = true;
    }
    else
    {
      m_AlwaysReact = false;
    }
  }
  else
  {
    m_AlwaysReact = false;
  }

  // auto repeat
  std::string strAutoRepeat = "";
  if (properties->GetStringProperty("autoRepeat", strAutoRepeat))
  {
    if (strAutoRepeat == "true")
    {
      m_AutoRepeat = true;
    }
    else
    {
      m_AutoRepeat = false;
    }
  }

  // pixel movement for scrolling one slice
  std::string strPixelPerSlice = "";
  if (properties->GetStringProperty("pixelPerSlice", strPixelPerSlice))
  {
    m_IndexToSliceModifier = atoi(strPixelPerSlice.c_str());
  }
  else
  {
    m_IndexToSliceModifier = 4;
  }

  // scroll direction
  if (!properties->GetStringProperty("scrollDirection", m_ScrollDirection))
  {
    m_ScrollDirection = "updown";
  }

  m_InvertScrollDirection = GetBoolProperty(properties, "invertScrollDirection", false);

  // zoom direction
  if (!properties->GetStringProperty("zoomDirection", m_ZoomDirection))
  {
    m_ZoomDirection = "updown";
  }

  m_InvertZoomDirection = GetBoolProperty(properties, "invertZoomDirection", false);

  m_InvertMoveDirection = GetBoolProperty(properties, "invertMoveDirection", false);

  if (!properties->GetStringProperty("levelWindowDirection", m_LevelDirection))
  {
    m_LevelDirection = "leftright";
  }

  m_InvertLevelWindowDirection = GetBoolProperty(properties, "invertLevelWindowDirection", false);

  // coupled rotation
  std::string strCoupled = "";
  if (properties->GetStringProperty("coupled", strCoupled))
  {
    if (strCoupled == "true")
    {
      m_LinkPlanes = true;
    }
    else
    {
      m_LinkPlanes = false;
    }
  }

  // zoom factor
  std::string strZoomFactor = "";
  properties->GetStringProperty("zoomFactor", strZoomFactor);
  m_ZoomFactor = .05;
  if (atoi(strZoomFactor.c_str()) > 0)
  {
    m_ZoomFactor = 1.0 + (atoi(strZoomFactor.c_str()) / 100.0);
  }
}

bool mitk::DisplayActionEventBroadcast::FilterEvents(InteractionEvent* interactionEvent, DataNode * /*dataNode*/)
{
  if (nullptr == interactionEvent->GetSender())
  {
    return false;
  }

  if (BaseRenderer::Standard3D == interactionEvent->GetSender()->GetMapperID())
  {
    return false;
  }

  return true;
}

bool mitk::DisplayActionEventBroadcast::CheckPositionEvent(const InteractionEvent *interactionEvent)
{
  const auto* positionEvent = dynamic_cast<const InteractionPositionEvent*>(interactionEvent);
  if (nullptr == positionEvent)
  {
    return false;
  }

  return true;
}

bool mitk::DisplayActionEventBroadcast::CheckRotationPossible(const InteractionEvent *interactionEvent)
{
  // Decide between moving and rotation slices.
  /*
  Detailed logic:

  1. Find the SliceNavigationController that has sent the event: this one defines our rendering plane and will NOT be
  rotated. Needs not even be counted or checked.
  2. Inspect every other SliceNavigationController
  - calculate the line intersection of this SliceNavigationController's plane with our rendering plane
  - if there is NO interesection, ignore and continue
  - IF there is an intersection
  - check the mouse cursor's distance from that line.
  0. if the line is NOT near the cursor, remember the plane as "one of the other planes" (which can be rotated in
  "locked" mode)
  1. on first line near the cursor,  just remember this intersection line as THE other plane that we want to rotate
  2. on every consecutive line near the cursor, check if the line is geometrically identical to the line that we want to
  rotate
  - if yes, we just push this line to the "other" lines and rotate it along
  - if no, then we have a situation where the mouse is near two other lines (e.g. crossing point) and don't want to
  rotate
  */
  const auto* posEvent = dynamic_cast<const InteractionPositionEvent*>(interactionEvent);
  if (nullptr == posEvent)
  {
    return false;
  }

  BaseRenderer* clickedRenderer = posEvent->GetSender();
  const PlaneGeometry* ourViewportGeometry = clickedRenderer->GetCurrentWorldPlaneGeometry();

  if (nullptr == ourViewportGeometry)
  {
    return false;
  }

  Point3D cursorPosition = posEvent->GetPositionInWorld();
  const auto spacing = ourViewportGeometry->GetSpacing();
  const PlaneGeometry *geometryToBeRotated = nullptr; // this one is under the mouse cursor
  const PlaneGeometry *anyOtherGeometry = nullptr;    // this is also visible (for calculation of intersection ONLY)
  Line3D intersectionLineWithGeometryToBeRotated;

  bool hitMultipleLines(false);
  m_SNCsToBeRotated.clear();

  const double threshholdDistancePixels = 12.0;

  auto renWindows = interactionEvent->GetSender()->GetRenderingManager()->GetAllRegisteredRenderWindows();

  for (auto renWin : renWindows)
  {
    SliceNavigationController *snc = BaseRenderer::GetInstance(renWin)->GetSliceNavigationController();

    // If the mouse cursor is in 3D Renderwindow, do not check for intersecting planes.
    if (BaseRenderer::GetInstance(renWin)->GetMapperID() == BaseRenderer::Standard3D)
      continue;

    const PlaneGeometry *otherRenderersRenderPlane = snc->GetCurrentPlaneGeometry();
    if (nullptr == otherRenderersRenderPlane)
    {
      continue; // ignore, we don't see a plane
    }

    // check if there is an intersection
    Line3D intersectionLine; // between rendered/clicked geometry and the one being analyzed
    if (!ourViewportGeometry->IntersectionLine(otherRenderersRenderPlane, intersectionLine))
    {
      continue; // we ignore this plane, it's parallel to our plane
    }

    // check distance from intersection line
    const double distanceFromIntersectionLine = intersectionLine.Distance(cursorPosition) / spacing[snc->GetDefaultViewDirection()];

    // far away line, only remember for linked rotation if necessary
    if (distanceFromIntersectionLine > threshholdDistancePixels)
    {
      anyOtherGeometry = otherRenderersRenderPlane; // we just take the last one, so overwrite each iteration (we just
                                                    // need some crossing point)
                                                    // TODO what about multiple crossings? NOW we have undefined behavior / random crossing point is used
      if (m_LinkPlanes)
      {
        m_SNCsToBeRotated.push_back(snc);
      }
    }
    else // close to cursor
    {
      if (nullptr == geometryToBeRotated) // first one close to the cursor
      {
        geometryToBeRotated = otherRenderersRenderPlane;
        intersectionLineWithGeometryToBeRotated = intersectionLine;
        m_SNCsToBeRotated.push_back(snc);
      }
      else
      {
        // compare to the line defined by geometryToBeRotated: if identical, just rotate this otherRenderersRenderPlane
        // together with the primary one
        //                                                     if different, DON'T rotate
        if (intersectionLine.IsParallel(intersectionLineWithGeometryToBeRotated) &&
            intersectionLine.Distance(intersectionLineWithGeometryToBeRotated.GetPoint1()) < mitk::eps)
        {
          m_SNCsToBeRotated.push_back(snc);
        }
        else
        {
          hitMultipleLines = true;
        }
      }
    }
  }

  bool moveSlices(true);

  if (geometryToBeRotated && anyOtherGeometry && ourViewportGeometry && !hitMultipleLines)
  {
    // assure all three are valid, so calculation of center of rotation can be done
    moveSlices = false;
  }
  // question in state machine is: "rotate?"
  if (moveSlices) // i.e. NOT rotate
  {
    return false;
  }
  else
  {
    // we DO have enough information for rotation
    // remember where the last cursor position ON THE LINE has been observed
    m_LastCursorPosition = intersectionLineWithGeometryToBeRotated.Project(cursorPosition);

    // find center of rotation by intersection with any of the OTHER lines
    if (anyOtherGeometry->IntersectionPoint(intersectionLineWithGeometryToBeRotated, m_CenterOfRotation))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}

bool mitk::DisplayActionEventBroadcast::CheckSwivelPossible(const InteractionEvent *interactionEvent)
{
  const ScalarType ThresholdDistancePixels = 6.0;

  // Decide between moving and rotation: if we're close to the crossing
  // point of the planes, moving mode is entered, otherwise
  // rotation/swivel mode
  const auto *posEvent = dynamic_cast<const InteractionPositionEvent *>(interactionEvent);

  BaseRenderer *renderer = interactionEvent->GetSender();

  if (!posEvent || !renderer)
  {
    return false;
  }

  const Point3D& cursor = posEvent->GetPositionInWorld();

  m_SNCsToBeRotated.clear();

  const PlaneGeometry* clickedGeometry(nullptr);
  const PlaneGeometry* otherGeometry1(nullptr);
  const PlaneGeometry* otherGeometry2(nullptr);

  auto registeredRenderWindows = interactionEvent->GetSender()->GetRenderingManager()->GetAllRegisteredRenderWindows();
  for (auto renWin : registeredRenderWindows)
  {
    SliceNavigationController* snc = BaseRenderer::GetInstance(renWin)->GetSliceNavigationController();

    // If the mouse cursor is in 3D Renderwindow, do not check for intersecting planes.
    if (BaseRenderer::GetInstance(renWin)->GetMapperID() == BaseRenderer::Standard3D)
      continue;

    // unsigned int slice = (*iter)->GetSlice()->GetPos();
    // unsigned int time  = (*iter)->GetTime()->GetPos();

    const PlaneGeometry *planeGeometry = snc->GetCurrentPlaneGeometry();
    if (!planeGeometry)
      continue;

    if (snc == renderer->GetSliceNavigationController())
    {
      clickedGeometry = planeGeometry;
      m_SNCsToBeRotated.push_back(snc);
    }
    else
    {
      if (otherGeometry1 == nullptr)
      {
        otherGeometry1 = planeGeometry;
      }
      else
      {
        otherGeometry2 = planeGeometry;
      }
      if (m_LinkPlanes)
      {
        // If planes are linked, apply rotation to all planes
        m_SNCsToBeRotated.push_back(snc);
      }
    }
  }

  Line3D line;
  Point3D point;
  if ((clickedGeometry != nullptr) && (otherGeometry1 != nullptr) && (otherGeometry2 != nullptr) &&
    clickedGeometry->IntersectionLine(otherGeometry1, line) && otherGeometry2->IntersectionPoint(line, point))
  {
    m_CenterOfRotation = point;
    if (m_CenterOfRotation.EuclideanDistanceTo(cursor) < ThresholdDistancePixels)
    {
      return false;
    }
    else
    {
      m_ReferenceCursor = posEvent->GetPointerPositionOnScreen();

      // Get main axes of rotation plane and store it for rotation step
      m_RotationPlaneNormal = clickedGeometry->GetNormal();

      ScalarType xVector[] = { 1.0, 0.0, 0.0 };
      ScalarType yVector[] = { 0.0, 1.0, 0.0 };
      clickedGeometry->BaseGeometry::IndexToWorld(Vector3D(xVector), m_RotationPlaneXVector);
      clickedGeometry->BaseGeometry::IndexToWorld(Vector3D(yVector), m_RotationPlaneYVector);

      m_RotationPlaneNormal.Normalize();
      m_RotationPlaneXVector.Normalize();
      m_RotationPlaneYVector.Normalize();

      m_PreviousRotationAxis.Fill(0.0);
      m_PreviousRotationAxis[2] = 1.0;
      m_PreviousRotationAngle = 0.0;

      return true;
    }
  }
  else
  {
    return false;
  }
  return false;
}

void mitk::DisplayActionEventBroadcast::Init(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  auto* positionEvent = static_cast<InteractionPositionEvent*>(interactionEvent);

  m_LastDisplayCoordinate = positionEvent->GetPointerPositionOnScreen();
  m_CurrentDisplayCoordinate = m_LastDisplayCoordinate;
  positionEvent->GetSender()->DisplayToPlane(m_LastDisplayCoordinate, m_StartCoordinateInMM);
  m_LastCoordinateInMM = m_StartCoordinateInMM;
}

void mitk::DisplayActionEventBroadcast::Move(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  auto* positionEvent = static_cast<InteractionPositionEvent*>(interactionEvent);

  BaseRenderer* sender = interactionEvent->GetSender();
  Vector2D moveVector = m_LastDisplayCoordinate - positionEvent->GetPointerPositionOnScreen();

  if (m_InvertMoveDirection)
  {
    moveVector *= -1.0;
  }

  moveVector *= sender->GetScaleFactorMMPerDisplayUnit(); // #TODO: put here?

  // store new display coordinate
  m_LastDisplayCoordinate = positionEvent->GetPointerPositionOnScreen();

  // propagate move event with computed geometry values
  InvokeEvent(DisplayMoveEvent(interactionEvent, moveVector));
}

void mitk::DisplayActionEventBroadcast::SetCrosshair(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  auto* positionEvent = static_cast<InteractionPositionEvent*>(interactionEvent);
  Point3D position = positionEvent->GetPositionInWorld();

  // propagate set crosshair event with computed geometry values
  InvokeEvent(DisplaySetCrosshairEvent(interactionEvent, position));
}

void mitk::DisplayActionEventBroadcast::Zoom(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  auto* positionEvent = static_cast<InteractionPositionEvent*>(interactionEvent);

  float factor = 1.0;
  float distance = 0;

  if (m_ZoomDirection == "updown")
  {
    distance = m_CurrentDisplayCoordinate[1] - m_LastDisplayCoordinate[1];
  }
  else
  {
    distance = m_CurrentDisplayCoordinate[0] - m_LastDisplayCoordinate[0];
  }

  if (m_InvertZoomDirection)
  {
    distance *= -1.0;
  }

  // set zooming speed
  if (distance < 0.0)
  {
    factor = 1.0 / m_ZoomFactor;
  }
  else if (distance > 0.0)
  {
    factor = 1.0 * m_ZoomFactor;
  }

  // store new display coordinates
  m_LastDisplayCoordinate = m_CurrentDisplayCoordinate;
  m_CurrentDisplayCoordinate = positionEvent->GetPointerPositionOnScreen();

  // propagate zoom event with computed geometry values
  InvokeEvent(DisplayZoomEvent(interactionEvent, factor, m_StartCoordinateInMM));
}

void mitk::DisplayActionEventBroadcast::Scroll(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  auto* positionEvent = static_cast<InteractionPositionEvent *>(interactionEvent);

  int sliceDelta = 0;

  // scroll direction
  if (m_ScrollDirection == "updown")
  {
    sliceDelta = static_cast<int>(m_LastDisplayCoordinate[1] - positionEvent->GetPointerPositionOnScreen()[1]);
  }
  else
  {
    sliceDelta = static_cast<int>(m_LastDisplayCoordinate[0] - positionEvent->GetPointerPositionOnScreen()[0]);
  }

  if (m_InvertScrollDirection)
  {
    sliceDelta *= -1;
  }

  // set how many pixels the mouse has to be moved to scroll one slice
  // if the mouse has been moved less than 'm_IndexToSliceModifier', pixels slice ONE slice only
  if (sliceDelta > 0 && sliceDelta < m_IndexToSliceModifier)
  {
    sliceDelta = m_IndexToSliceModifier;
  }
  else if (sliceDelta < 0 && sliceDelta > -m_IndexToSliceModifier)
  {
    sliceDelta = -m_IndexToSliceModifier;
  }
  sliceDelta /= m_IndexToSliceModifier;

  // store new display coordinates
  m_LastDisplayCoordinate = m_CurrentDisplayCoordinate;
  m_CurrentDisplayCoordinate = positionEvent->GetPointerPositionOnScreen();

  // propagate scroll event with computed geometry values
  InvokeEvent(DisplayScrollEvent(interactionEvent, sliceDelta));
}

void mitk::DisplayActionEventBroadcast::ScrollOneUp(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  int sliceDelta = 1;
  if (m_InvertScrollDirection)
  {
    sliceDelta = -1;
  }

  // propagate scroll event with a single slice delta (increase)
  InvokeEvent(DisplayScrollEvent(interactionEvent, sliceDelta));
}

void mitk::DisplayActionEventBroadcast::ScrollOneDown(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  int sliceDelta = -1;
  if (m_InvertScrollDirection)
  {
    sliceDelta = 1;
  }
  // propagate scroll event with a single slice delta (decrease)
  InvokeEvent(DisplayScrollEvent(interactionEvent, sliceDelta));
}

void mitk::DisplayActionEventBroadcast::AdjustLevelWindow(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  // nothing here; no event sent
}

void mitk::DisplayActionEventBroadcast::StartRotation(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  // nothing here; no event sent
}

void mitk::DisplayActionEventBroadcast::EndRotation(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{
  // nothing here; no event sent
}

void mitk::DisplayActionEventBroadcast::Rotate(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{  
  // nothing here; no event sent
}

void mitk::DisplayActionEventBroadcast::Swivel(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{  
  // nothing here; no event sent
}

void mitk::DisplayActionEventBroadcast::UpdateStatusbar(StateMachineAction* stateMachineAction, InteractionEvent* interactionEvent)
{

}

bool mitk::DisplayActionEventBroadcast::GetBoolProperty(mitk::PropertyList::Pointer propertyList, const char* propertyName, bool defaultValue)
{
  std::string valueAsString;
  if (!propertyList->GetStringProperty(propertyName, valueAsString))
  {
    return defaultValue;
  }
  else
  {
    if (valueAsString == "true")
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

mitk::DataNode::Pointer mitk::DisplayActionEventBroadcast::GetTopLayerNode(mitk::DataStorage::SetOfObjects::ConstPointer nodes, mitk::Point3D worldposition, BaseRenderer* renderer)
{
  // #TODO: see T24173
  return nodes->front();
}
