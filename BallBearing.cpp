
#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>
#include <CAM/CAMAll.h>

#include <Core/Utils.h>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace adsk::core;
using namespace adsk::fusion;
using namespace adsk::cam;

Ptr<Application> gptrApp;
Ptr<UserInterface> gptrUi;
std::string gstrUnits = "";

// Global command input declarations.
Ptr<ValueCommandInput> gptrInnerDiameter;
Ptr<ValueCommandInput> gptrOuterDiameter;
Ptr<ValueCommandInput> gptrThickness;
Ptr<TextBoxCommandInput> gptrErrorMessage;

bool getCommandInputValue(Ptr<CommandInput> commandInput, std::string unitType, double *value);
Ptr<Component> drawBallBearing(Ptr<Design> design, double innerDiameter, double outerDiameter, double thickness);


bool checkReturn(Ptr<Base> returnObj)
{
	if (returnObj)
		return true;
	else
		if (gptrApp && gptrUi)
		{
			std::string errDesc;
			gptrApp->getLastError(&errDesc);
			gptrUi->messageBox(errDesc);
			return false;
		}
		else
			return false;
}

// Event handler for the execute event.
class GearCommandExecuteEventHandler : public adsk::core::CommandEventHandler
{
public:
	void notify(const Ptr<CommandEventArgs>& eventArgs) override
	{
		// Save the current values as attributes.
		Ptr<Design> des = gptrApp->activeProduct();
		Ptr<Attributes> attribs = des->attributes();
		attribs->add("BallBearing", "innerDiameter", std::to_string(gptrInnerDiameter->value()));
		attribs->add("BallBearing", "outerDiameter", std::to_string(gptrOuterDiameter->value()));
		attribs->add("BallBearing", "thickness", std::to_string(gptrThickness->value()));

		//int numTeeth = std::stoi(_numTeeth->value());
		double dInnerDiameter = gptrInnerDiameter->value();
		double dOuterDiameter = gptrOuterDiameter->value();
		double dThickness = gptrThickness->value();

		// Create the gear.
		Ptr<Component> ptrCmpBearing;
		ptrCmpBearing = drawBallBearing(des, dInnerDiameter, dOuterDiameter, dThickness);

		if (ptrCmpBearing)
		{
			std::string desc = "";
			desc += "Inner Diameter: " + std::to_string(dInnerDiameter) + "; ";
			desc += "Outer Diameter: " + std::to_string(dOuterDiameter) + "; ";
			desc += "Thickness: " + std::to_string(dThickness) + "; ";

			ptrCmpBearing->description(desc);
		}
		else
		{
			eventArgs->executeFailed(true);
			eventArgs->executeFailedMessage("Unexpected failure while constructing the ball bearing.");
		}
	}
} gCmdExecute;


class GearCommandInputChangedHandler : public adsk::core::InputChangedEventHandler
{
public:
	void notify(const Ptr<InputChangedEventArgs>& eventArgs) override
	{
		Ptr<CommandInput> changedInput = eventArgs->input();
	}
} gCmdInputChanged;


class GearCommandValidateInputsEventHandler : public adsk::core::ValidateInputsEventHandler
{
public:
	void notify(const Ptr<ValidateInputsEventArgs>& eventArgs) override
	{
		gptrErrorMessage->text("");

		double dInnerDiameter;
		double dOuterDiameter;
		double dThickness;

		double value;
		if (!getCommandInputValue(gptrInnerDiameter, gstrUnits, &value)) {
			eventArgs->areInputsValid(false);
			return;
		} else {
			dInnerDiameter = value;
		}
		if (!getCommandInputValue(gptrOuterDiameter, gstrUnits, &value)) {
			eventArgs->areInputsValid(false);
			return;
		} else {
			dOuterDiameter = value;
		}
		if (!getCommandInputValue(gptrThickness, gstrUnits, &value)) {
			eventArgs->areInputsValid(false);
			return;
		} else {
			dThickness = value;
		}

		if (dInnerDiameter >= dOuterDiameter) {
			gptrErrorMessage->text("Inner diameter cannot be bigger than the outer one.");
			eventArgs->areInputsValid(false);
			return;
		}
		if (dThickness <= 0.0) {
			gptrErrorMessage->text("Thickness value needs to be bigger than 0.0");
			eventArgs->areInputsValid(false);
			return;
		}
	}
} gCmdValidateInputs;


class SpurGearCommandCreatedEventHandler : public adsk::core::CommandCreatedEventHandler
{
public:
	void notify(const Ptr<CommandCreatedEventArgs>& eventArgs) override
	{
		// Verify that a Fusion design is active.
		Ptr<Design> ptrDesign = gptrApp->activeProduct();
		if (!checkReturn(ptrDesign)) {
			gptrUi->messageBox("A Fusion design must be active when invoking this command.");
			return;
		}

		std::string strDefaultUnits = ptrDesign->unitsManager()->defaultLengthUnits();

		// Determine whether to use inches or millimeters as the intial default.
		if (strDefaultUnits == "in" || strDefaultUnits == "ft") {
			gstrUnits = "in";
		} else {
			gstrUnits = "mm";
		}

		// Define the default values and get the previous values from the attributes.

		std::string strInnerDiameter = std::to_string(10.0);
		Ptr<Attribute> ptrAttrInnerDiameter = ptrDesign->attributes()->itemByName("BallBearing", "innerDiameter");
		if (checkReturn(ptrAttrInnerDiameter))
			strInnerDiameter = ptrAttrInnerDiameter->value();

		std::string strOuterDiameter = std::to_string(20.0);
		Ptr<Attribute> ptrAttrOuterDiameter = ptrDesign->attributes()->itemByName("BallBearing", "outerDiameter");
		if (checkReturn(ptrAttrOuterDiameter))
			strOuterDiameter = ptrAttrOuterDiameter->value();

		std::string strThickness = std::to_string(5.0);
		Ptr<Attribute> ptrAttrThickness = ptrDesign->attributes()->itemByName("BallBearing", "thickness");
		if (checkReturn(ptrAttrThickness))
			strThickness = ptrAttrThickness->value();

		Ptr<Command> ptrCmd = eventArgs->command();
		ptrCmd->isExecutedWhenPreEmpted(false);
		Ptr<CommandInputs> inputs = ptrCmd->commandInputs();
		if (!checkReturn(inputs))
			return;

		// Define the command dialog.
		gptrInnerDiameter = inputs->addValueInput("innerDiameter", "Inner Diameter", gstrUnits, ValueInput::createByReal(std::stod(strInnerDiameter)));
		if (!checkReturn(gptrInnerDiameter))
			return;

		gptrOuterDiameter = inputs->addValueInput("outerDiameter", "Outer Diameter", gstrUnits, ValueInput::createByReal(std::stod(strOuterDiameter)));
		if (!checkReturn(gptrOuterDiameter))
			return;

		gptrThickness = inputs->addValueInput("thickness", "Thickness", gstrUnits, ValueInput::createByReal(std::stod(strThickness)));
		if (!checkReturn(gptrThickness))
			return;

		gptrErrorMessage = inputs->addTextBoxCommandInput("errMessage", "", "", 2, true);
		if (!checkReturn(gptrErrorMessage))
			return;
		gptrErrorMessage->isFullWidth(true);

		// Connect to the command related events.
		Ptr<InputChangedEvent> ptrEventInputChanged = ptrCmd->inputChanged();
		if (!ptrEventInputChanged)
			return;
		bool isOk = ptrEventInputChanged->add(&gCmdInputChanged);
		if (!isOk)
			return;

		Ptr<ValidateInputsEvent> ptrEventValidateInputs = ptrCmd->validateInputs();
		if (!ptrEventValidateInputs)
			return;
		isOk = ptrEventValidateInputs->add(&gCmdValidateInputs);
		if (!isOk)
			return;

		Ptr<CommandEvent> ptrEventExecute = ptrCmd->execute();
		if (!ptrEventExecute)
			return;
		isOk = ptrEventExecute->add(&gCmdExecute);
		if (!isOk)
			return;
	}
} gCmdCreated;

/*
 *  Verfies that a value command input has a valid expression and returns the
 *  value if it does.  Otherwise it returns False.  This works around a
 *  problem where when you get the value from a ValueCommandInput it causes the
 *  current expression to be evaluated and updates the display.  Some new functionality
 *  is being added in the future to the ValueCommandInput object that will make
 *  this easier and should make this function obsolete.
 */
bool getCommandInputValue(Ptr<CommandInput> commandInput, std::string unitType, double *value)
{
	Ptr<ValueCommandInput> valCommandInput = commandInput;
	if (!commandInput) {
		*value = 0;
		return false;
	}

	// Verify that the expression is valid.
	Ptr<Design> des = gptrApp->activeProduct();
	Ptr<UnitsManager> unitsMgr = des->unitsManager();

	if (unitsMgr->isValidExpression(valCommandInput->expression(), unitType)) {
		*value = unitsMgr->evaluateExpression(valCommandInput->expression(), unitType);
		return true;
	} else {
		*value = 0;
		return false;
	}
}

Ptr<Component> generateComponent(Ptr<Design> design) {
	// Create a new component by creating an occurrence.
	Ptr<Occurrences> ptrOccs = design->rootComponent()->occurrences();
	if (!checkReturn(ptrOccs))
		return nullptr;

	Ptr<Matrix3D> ptrMat = adsk::core::Matrix3D::create();
	if (!checkReturn(ptrMat))
		return nullptr;

	Ptr<Occurrence> ptrNewOcc = ptrOccs->addNewComponent(ptrMat);
	if (!checkReturn(ptrNewOcc))
		return nullptr;

	Ptr<Component> ptrNewComp = ptrNewOcc->component();
	if (!checkReturn(ptrNewComp))
		return nullptr;

	return ptrNewComp;
}

Ptr<Sketch> drawBallCutoutSketch(Ptr<Sketches> sketches, Ptr<ConstructionPlane> plane, double radius, double offset) {
	Ptr<Sketch> ptrSketchBallsCutout = sketches->add(plane);
	if (!checkReturn(ptrSketchBallsCutout))
		return nullptr;
	ptrSketchBallsCutout->sketchCurves()->sketchCircles()->addByCenterRadius(
		adsk::core::Point3D::create(offset, 0, 0.0),
		radius
	);
	return ptrSketchBallsCutout;
}

Ptr<Sketch> drawInnerRingSketch(Ptr<Sketches> sketches, Ptr<ConstructionPlane> plane, double radius, double ringWidth, double thickness) {
	Ptr<Sketch> ptrSketchInnerRing = sketches->add(plane);
	if (!checkReturn(ptrSketchInnerRing))
		return nullptr;
	ptrSketchInnerRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius, -thickness * 0.5, 0),
		adsk::core::Point3D::create(radius, thickness * 0.5, 0));
	ptrSketchInnerRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius, thickness * 0.5, 0),
		adsk::core::Point3D::create(radius + ringWidth, thickness * 0.5, 0));
	ptrSketchInnerRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius + ringWidth, thickness * 0.5, 0),
		adsk::core::Point3D::create(radius + ringWidth, -thickness * 0.5, 0));
	ptrSketchInnerRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius + ringWidth, -thickness * 0.5, 0),
		adsk::core::Point3D::create(radius, -thickness * 0.5, 0));

	return ptrSketchInnerRing;
}
Ptr<Sketch> drawOuterRingSketch(Ptr<Sketches> sketches, Ptr<ConstructionPlane> plane, double radius, double ringWidth, double thickness) {
	Ptr<Sketch> ptrSketchOuterRing = sketches->add(plane);
	if (!checkReturn(ptrSketchOuterRing))
		return nullptr;
	ptrSketchOuterRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius, -thickness * 0.5, 0),
		adsk::core::Point3D::create(radius, thickness * 0.5, 0));
	ptrSketchOuterRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius, thickness * 0.5, 0),
		adsk::core::Point3D::create(radius - ringWidth, thickness * 0.5, 0));
	ptrSketchOuterRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius - ringWidth, thickness * 0.5, 0),
		adsk::core::Point3D::create(radius - ringWidth, -thickness * 0.5, 0));
	ptrSketchOuterRing->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(radius - ringWidth, -thickness * 0.5, 0),
		adsk::core::Point3D::create(radius, -thickness * 0.5, 0));

	return ptrSketchOuterRing;
}

Ptr<RevolveFeature> createComponentWithRevolve(Ptr<Component> component, Ptr<Sketch> sketch, Ptr<ConstructionAxis> axis) {
	Ptr<Profile> ptrProfile = nullptr;

	ptrProfile = sketch->profiles()->item(0);
	if (!checkReturn(ptrProfile))
		return nullptr;

	Ptr<RevolveFeatures> ptrRevolves = component->features()->revolveFeatures();
	if (!checkReturn(ptrRevolves))
		return nullptr;

	Ptr<RevolveFeatureInput> ptrRevolveInput = ptrRevolves->createInput(ptrProfile, axis, NewComponentFeatureOperation);
	if (!checkReturn(ptrRevolveInput))
		return nullptr;

	// Define that the revolve is a full circle
	Ptr<ValueInput> ptrAngle = adsk::core::ValueInput::createByReal(360.0);
	if (!checkReturn(ptrAngle))
		return nullptr;

	bool xResult = ptrRevolveInput->setAngleExtent(false, ptrAngle);
	if (!xResult)
		return nullptr;

	// Create the revolve.
	Ptr<RevolveFeature> ptrRevolve = ptrRevolves->add(ptrRevolveInput);
	if (!checkReturn(ptrRevolve))
		return nullptr;

	return ptrRevolve;
}

bool applyFilletToRevolve(Ptr<Component> component, Ptr<RevolveFeature> revolve, double filletRadius) {
	Ptr<ObjectCollection> ptrColEdges = adsk::core::ObjectCollection::create();
	Ptr<FilletFeature> ptrFillet;
	Ptr<BRepFaces> ptrFaces = revolve->faces();
	for (Ptr<BRepFace> face : ptrFaces) {
		for (Ptr<BRepEdge> edge : face->edges()) {
			ptrColEdges->add(edge);
		}
	}
	// Create a fillet input to be able to define the input needed for a fillet.
	Ptr<FilletFeatures> ptrFillets = component->features()->filletFeatures();
	if (!checkReturn(ptrFillets))
		return false;

	Ptr<FilletFeatureInput> ptrFilletInput = ptrFillets->createInput();
	if (!checkReturn(ptrFilletInput))
		return false;

	// Define fillet radius
	Ptr<ValueInput> ptrRadius = adsk::core::ValueInput::createByReal(filletRadius);
	if (!checkReturn(ptrRadius))
		return false;

	bool xResult = ptrFilletInput->addConstantRadiusEdgeSet(ptrColEdges, ptrRadius, false);
	if (!xResult)
		return false;

	// Create the extrusion.
	ptrFillet = ptrFillets->add(ptrFilletInput);
	if (!checkReturn(ptrFillet))
		return false;

	return true;
}
Ptr<RevolveFeature> applyRevolveCut(Ptr<Component> component, Ptr<Sketch> sketch, Ptr<ConstructionAxis> axis) {
	Ptr<Profile> ptrProfile = nullptr;

	ptrProfile = sketch->profiles()->item(0);
	if (!checkReturn(ptrProfile))
		return nullptr;

	Ptr<RevolveFeatures> ptrRevolves = component->features()->revolveFeatures();
	if (!checkReturn(ptrRevolves))
		return nullptr;

	Ptr<RevolveFeatureInput> ptrRevolveInput = ptrRevolves->createInput(ptrProfile, axis, CutFeatureOperation);
	if (!checkReturn(ptrRevolveInput))
		return nullptr;

	// Define that the revolve is a full circle
	Ptr<ValueInput> ptrAngle = adsk::core::ValueInput::createByReal(360.0);
	if (!checkReturn(ptrAngle))
		return nullptr;

	bool xResult = ptrRevolveInput->setAngleExtent(false, ptrAngle);
	if (!xResult)
		return nullptr;

	// Create the revolve.
	Ptr<RevolveFeature> ptrRevolve = ptrRevolves->add(ptrRevolveInput);
	if (!checkReturn(ptrRevolve))
		return nullptr;

	return ptrRevolve;
}

bool createBalls(Ptr<Component> newComp, Ptr<RevolveFeature> innerRing, double ballRadius, double ballsOffset) {
	Ptr<Sketch> ptrBallSketch = newComp->sketches()->add(newComp->xZConstructionPlane());
	if (!checkReturn(ptrBallSketch))
		return nullptr;
	ptrBallSketch->sketchCurves()->sketchArcs()->addByCenterStartSweep(
		adsk::core::Point3D::create(ballsOffset, 0, 0.0),
		adsk::core::Point3D::create(ballsOffset - ballRadius, 0, 0.0),
		180.0
	);
	ptrBallSketch->sketchCurves()->sketchLines()->addByTwoPoints(
		adsk::core::Point3D::create(ballsOffset + ballRadius, 0, 0.0),
		adsk::core::Point3D::create(ballsOffset - ballRadius, 0, 0.0)
	);

	Ptr<Profile> ptrProfile = nullptr;

	ptrProfile = ptrBallSketch->profiles()->item(0);
	if (!checkReturn(ptrProfile))
		return false;

	Ptr<RevolveFeatures> ptrRevolves = newComp->features()->revolveFeatures();
	if (!checkReturn(ptrRevolves))
		return false;

	Ptr<RevolveFeatureInput> ptrRevolveInput = ptrRevolves->createInput(ptrProfile, newComp->xConstructionAxis(), NewBodyFeatureOperation);
	if (!checkReturn(ptrRevolveInput))
		return false;

	// Define that the revolve is a full circle
	Ptr<ValueInput> ptrAngle = adsk::core::ValueInput::createByReal(360.0);
	if (!checkReturn(ptrAngle))
		return false;

	bool xResult = ptrRevolveInput->setAngleExtent(false, ptrAngle);
	if (!xResult)
		return false;

	// Create the revolve.
	Ptr<RevolveFeature> ptrRevolve = ptrRevolves->add(ptrRevolveInput);
	if (!checkReturn(ptrRevolve))
		return false;

	// Circular pattern.
	Ptr<CircularPatternFeatures> ptrCircPatterns = newComp->features()->circularPatternFeatures();
	if (!checkReturn(ptrCircPatterns))
		return false;

	Ptr<ObjectCollection> ptrColEntities = adsk::core::ObjectCollection::create();
	ptrColEntities->add(ptrRevolve);

	Ptr<CircularPatternFeatureInput> ptrPatternInput = ptrCircPatterns->createInput(ptrColEntities, newComp->zConstructionAxis());
	if (!checkReturn(ptrPatternInput))
		return false;

	int iBallCount = (int)(2.0 * 3.141592 * ballsOffset / (ballRadius * 2.0)) - 1;
	Ptr<ValueInput> ptrBallCount = adsk::core::ValueInput::createByString(std::to_string(iBallCount));
	if (!checkReturn(ptrBallCount))
		return false;

	ptrPatternInput->quantity(ptrBallCount);
	Ptr<CircularPatternFeature> ptrPattern = ptrCircPatterns->add(ptrPatternInput);
	if (!checkReturn(ptrPattern))
		return false;

	return true;
}

// Builds a ball bearing.
Ptr<Component> drawBallBearing(Ptr<Design> design, double innerDiameter, double outerDiameter, double thickness)
{
	Ptr<Component> ptrNewComp = generateComponent(design);
	if (!checkReturn(ptrNewComp))
		return nullptr;

	// Create a new sketch.
	Ptr<Sketches> ptrSketches = ptrNewComp->sketches();
	if (!checkReturn(ptrSketches))
		return nullptr;

	Ptr<ConstructionPlane> ptrXyPlane = ptrNewComp->xZConstructionPlane();
	if (!checkReturn(ptrXyPlane))
		return nullptr;

	Ptr<ConstructionAxis> ptrZAxis = ptrNewComp->zConstructionAxis();

	double dBallRadius = thickness * 0.6;
	if (dBallRadius > ((outerDiameter - innerDiameter) * 0.5 * 0.6)) {
		dBallRadius = (outerDiameter - innerDiameter) * 0.5 * 0.6;
	}
	dBallRadius *= 0.5;

	double dRingWidth = (outerDiameter - innerDiameter) * 0.25 - dBallRadius * 0.5;

	// Draw sketch for the ball cutout
	Ptr<Sketch> ptrSketchBallsCutout = drawBallCutoutSketch(ptrSketches, ptrXyPlane, dBallRadius, (outerDiameter + innerDiameter) * 0.5 * 0.5);

	// Draw sketch of the inner ring
	Ptr<Sketch> ptrSketchInnerRing = drawInnerRingSketch(ptrSketches, ptrXyPlane, innerDiameter * 0.5, dRingWidth, thickness);

	// Draw outer ring sketch
	Ptr<Sketch> ptrSketchOuterRing = drawOuterRingSketch(ptrSketches, ptrXyPlane, outerDiameter * 0.5, dRingWidth, thickness);

	//////// Revolve the profiles to get rings

	// Create a revolve input to be able to define the input needed for an revolve
	// while specifying the profile and that a new component is to be created

	// Inner Ring
	// Create the revolve.
	Ptr<RevolveFeature> ptrRevolveInnerRing = createComponentWithRevolve(ptrNewComp, ptrSketchInnerRing, ptrZAxis);
	if (!checkReturn(ptrRevolveInnerRing))
		return nullptr;
	Ptr<Component> ptrCmpInnerRing = ptrRevolveInnerRing->parentComponent();
	ptrCmpInnerRing->name("Inner Ring");

	// Fillet 
	if (!applyFilletToRevolve(ptrNewComp, ptrRevolveInnerRing, dRingWidth * 0.1)) {
		return nullptr;
	}

	// Outer Ring
	// Create the revolve.
	Ptr<RevolveFeature> ptrRevolveOuterRing = createComponentWithRevolve(ptrNewComp, ptrSketchOuterRing, ptrZAxis);
	if (!checkReturn(ptrRevolveOuterRing))
		return nullptr;
	ptrRevolveOuterRing->parentComponent()->name("Outer Ring");

	// Fillet 
	if (!applyFilletToRevolve(ptrNewComp, ptrRevolveOuterRing, dRingWidth * 0.1)) {
		return nullptr;
	}

	// Balls cutout
	// Create the cut.
	Ptr<RevolveFeature> ptrRevolveBallsCutout = applyRevolveCut(ptrNewComp, ptrSketchBallsCutout, ptrZAxis);
	if (!checkReturn(ptrRevolveBallsCutout))
		return nullptr;

	// Add balls
	if (!createBalls(ptrNewComp, ptrRevolveInnerRing, dBallRadius, (innerDiameter + outerDiameter) * 0.5 * 0.5)) {
		return nullptr;
	}

	// Create joint
	// Create the first joint geometry with the side face
	Ptr<JointGeometry> ptrJntGeometryInner = JointGeometry::createByNonPlanarFace(ptrRevolveInnerRing->faces()->item(2), StartKeyPoint);
	if (!checkReturn(ptrJntGeometryInner))
		return nullptr;

	// Create the second joint geometry with prof1
	Ptr<JointGeometry> ptrJntGeometryOuter = JointGeometry::createByNonPlanarFace(ptrRevolveOuterRing->faces()->item(0), StartKeyPoint);
	if (!checkReturn(ptrJntGeometryOuter))
		return nullptr;

	// Create joint input
	Ptr<Joints> ptrJoints = ptrNewComp->joints();
	if (!checkReturn(ptrJoints))
		return nullptr;
	Ptr<JointInput> ptrJntInput = ptrJoints->createInput(ptrJntGeometryInner, ptrJntGeometryOuter);
	if (!checkReturn(ptrJntInput))
		return nullptr;

	// Set the joint input
	ptrJntInput->isFlipped(false);
	ptrJntInput->setAsRevoluteJointMotion(JointDirections::ZAxisJointDirection);

	// Create the joint
	Ptr<Joint> ptrJoint = ptrJoints->add(ptrJntInput);
	if (!checkReturn(ptrJoint))
		return nullptr;


	// Set name
	ptrNewComp->name("Ball Bearing (" + std::to_string(innerDiameter) + " : " + std::to_string(outerDiameter) + ")");

	return ptrNewComp;
}


extern "C" XI_EXPORT bool run(const char* context)
{
	gptrApp = Application::get();
	if (!gptrApp)
		return false;

	gptrUi = gptrApp->userInterface();
	if (!gptrUi)
		return false;

	// Create a command definition and add a button to the CREATE panel.
	Ptr<CommandDefinition> cmdDef = gptrUi->commandDefinitions()->itemById("asBallBearingCPPScript");
	if (!cmdDef) {
		cmdDef = gptrUi->commandDefinitions()->addButtonDefinition("asBallBearingCPPScript", "Ball Bearing", "Creates a ball bearing component", "Resources/BallBearing");
		if (!checkReturn(cmdDef))
			return false;
	}

	Ptr<ToolbarPanel> createPanel = gptrUi->allToolbarPanels()->itemById("SolidCreatePanel");
	if (!checkReturn(createPanel))
		return false;

	Ptr<CommandControl> ptrBallBearing = createPanel->controls()->addCommand(cmdDef);
	if (!checkReturn(ptrBallBearing))
		return false;

	// Connect to the command created event.
	Ptr<CommandCreatedEvent> commandCreatedEvent = cmdDef->commandCreated();
	if (!checkReturn(commandCreatedEvent))
		return false;
	bool isOk = commandCreatedEvent->add(&gCmdCreated);
	if (!isOk)
		return false;

	isOk = cmdDef->execute();
	if (!isOk)
		return false;

	std::string strContext = context;
	if (strContext.find("IsApplicationStartup", 0) != std::string::npos)
	{
		if (strContext.find("false", 0) != std::string::npos)
		{
			gptrUi->messageBox("The \"Ball Bearing\" command has been added\nto the CREATE panel of the MODEL workspace.");
		}
	}

	// Prevent this module from terminating so that the command can continue to run until
	// the user completes the command.
	adsk::autoTerminate(false);

	//ui->messageBox("Hello addin");

	return true;
}

extern "C" XI_EXPORT bool stop(const char* context)
{
	if (gptrUi) 
	{
		Ptr<ToolbarPanel> createPanel = gptrUi->allToolbarPanels()->itemById("SolidCreatePanel");
		if (!checkReturn(createPanel))
			return false;

		Ptr<CommandControl> gearButton = createPanel->controls()->itemById("asBallBearingCPPScript");
		if (checkReturn(gearButton))
			gearButton->deleteMe();

		Ptr<CommandDefinition> cmdDef = gptrUi->commandDefinitions()->itemById("asBallBearingCPPScript");
		if (checkReturn(cmdDef))
			cmdDef->deleteMe();

		gptrUi->messageBox("Ball Bearing add-in has been stopped.");
		gptrUi = nullptr;
	}

	return true;
}


#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // XI_WIN
