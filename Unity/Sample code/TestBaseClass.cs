/*
 * TestBaseClass
 * 
 * The base class for all humphrey style tests. 
 * The test first will build the test points configured in start and then start the test. 
 * The test locks to the headset by default.  
 */


using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Testing
{
    public class TestBaseClass : MonoBehaviour
    {
        protected Quadrant QuadBeingCentered = Quadrant.Center;
        protected EyeTestChoice testChoice = EyeTestChoice.None;

        /// <summary>
        /// The layer for the eye's colission sphere. 
        /// </summary>
        protected const int LAYER_LEFT = 1 << 11, LAYER_RIGHT = 1 << 14;

        /// <summary>
        /// lightFound: A light was found along the grid. Will return false until light is found or grid is being moved. 
        /// </summary>
        protected bool lightFound = false;
        /// <summary>
        /// m_TestPaused: Test will be paused when the 
        /// </summary>
        protected bool m_TestPaused = false;

        /// <summary>
        /// CenterPointCentered: The center point is in the center of the screen if false.
        /// </summary>
        protected bool CenterPointCentered = false;

        /// <summary>
        /// coroutineComplete: said coroutine is complete
        /// </summary>
        protected bool coroutineComplete = false;
        /// <summary>
        /// ListsClearedAndLightsRemoved: This is true after the test is complete and
        /// the test is clearing out extra data used by test.
        /// </summary>
        protected bool ListsClearedAndLightsRemoved = false;

        /// <summary>
        /// Current light set to point at from the center of the grid
        /// </summary>
        protected BlinkingLight moveTolight, chosenLight;

        /// <summary>
        /// Grid positioning for where the b   bb
        /// </summary>
        protected Vector3 FinalCenterPos, OriginalCenter;

        /// <summary>
        /// List of the center positions for x number of frames
        /// </summary>
        protected Queue<Vector3> CenterPosition = new Queue<Vector3>();

        /// <summary>
        /// Light indexes for what light is currently being tested and the previous light that is being tested
        /// </summary>
        protected int lightIndex = -1;

        /// <summary>
        /// Variables that can be changed in the child classes to change how certian grids are built.
        /// </summary>
        protected int quadLightCount = 12, gridSize = 3;

        /// <summary>
        /// information used for the light grid building. 
        /// </summary>
        protected float StartingDegreeOutward, degreeOfSeparation;

        /// <summary>
        /// Used for eye tracking, Enables pausing of the test while eyes are blinking
        /// </summary>
        protected bool currentlyBlinking = false;

        /// <summary>
        /// totalBlinks
        ///
        /// The amount of blinks a user makes
        /// </summary>
        protected int totalBlinks = 0;

        /// <summary>
        /// WhatCanBeSeenCompleted
        ///
        /// A bool that is true when the lights that can be seen function has completed
        /// </summary>
        protected bool WhatCanBeSeenCompleted = false;

        /// <summary>
        /// GameObjects in the scene that will need modification
        ///
        /// blinkingLightLeft/RightPrefab: The base for what a blinking light should be
        ///
        /// centerPoint: the center of the vision test. This needs to be scaled and set to the correct position.
        ///
        /// EyePoint: the fixation point on where the eyes meet.
        /// </summary>
        public GameObject blinkingLightLeftPrefab, blinkingLightRightPrefab, blinkingBoxLeftPrefab, blinkingBoxRightPrefab, centerPoint, EyePoint, VisCross;

        /// <summary>
        /// The necessary lists for the blinking lights test to run properly.
        /// </summary>
        public List<BlinkingLight> blinkingLights, blinkingLightsSeed, blinkingLightsSeedForever,
                                  blinkingLightsActive, blinkingLightsUL, blinkingLightsUR, blinkingLightsBL,
                                  blinkingLightsBR, blinkingLightsLeft, blinkingLightsRight, bLightOutlier,
                                  LightsSeen, LightsSeenSeed, LightsSeenOuter, LightsSeenOutlier, BlinkingBoxes, BlinkingBoxesActive;

        /// <summary>
        /// The GameObjects of the lights
        /// </summary>
        public List<GameObject> lightObjects, m_BoxObjects;

        /// <summary>
        /// lightsAndListsCoroutineComplete:
        /// </summary>
        public bool lightsAndListsCoroutineComplete = false;
        /// <summary>
        /// CenterPointMovementActivated:
        /// </summary>
        public bool CenterPointMovementActivated = false;
        /// <summary>
        /// TestStarted: The test is actively running if true
        /// </summary>
        public bool TestStarted = false;
        /// <summary>
        /// MovementSetToQuad:
        /// </summary>
        public bool MovementSetToQuad = false;
        /// <summary>
        /// isCompleted: The Test is complete
        /// </summary>
        public bool isCompleted = false;
        /// <summary>
        /// FalsePosNegTestsActive:
        /// </summary>
        public bool FalsePosNegTestsActive = false;
        /// <summary>
        /// pausingActivated:
        /// </summary>
        public bool pausingActivated = false;
        /// <summary>
        /// testRunning: 
        /// </summary>
        public bool testRunning = false;
        /// <summary>
        /// eyeTrackingActive:
        /// </summary>
        public bool eyeTrackingActive = false;
        /// <summary>
        /// BoxesActive: 
        /// </summary>
        public bool BoxesActive = false;

        /// <summary>
        /// Graph Speed
        ///
        /// The speed that which the graph  will center to the head
        /// </summary>
        public float GraphSpeed = 15f;

        /// <summary>
        /// ParticleSystem used to display if the light was hit or not.
        /// FOR TESTING PURPOSES ONLY
        /// </summary>
        public ParticleSystem particle;

        /// <summary>
        /// Center location for moved grid and original center location
        ///
        /// LightFromCenterDiff: the difference between the current center of the grid to the new center point.
        /// </summary>
        public Vector3 LightFromCenterDiff = new Vector3(), newCenter = new Vector3();

        /// <summary>
        /// left, right cameras and the camera set for sight per eye. The View camera has a smaller FOV so that the view per eye is seen correctly.
        /// </summary>
        public Camera lCamera, rCamera, viewCameraL, viewCameraR;

        /// <summary>
        /// falseNegative 
        /// </summary>
        public FalseNegative falseNegative;

        /// <summary>
        /// falsePositive 
        /// </summary>
        public FalsePositive falsePositive;

        protected bool GridMoved = true;

        /// <summary>
        /// eyeManager 
        /// </summary>
        public MLEyeManager eyeManager;

        /// <summary>
        /// Arows that notify user to look at crosshair
        /// </summary>
        public ArrowPointer arrowPointer;

        /// <summary>
        /// Set grid size here and enable center point. Also call global starting procedures. 
        /// </summary>
        private void Start()
        {
            centerPoint.GetComponent<MeshRenderer>().enabled = true;
            GlobalStartingProcedures();
            quadLightCount = 13;
            gridSize = 3;
            StartingDegreeOutward = 3f;
            degreeOfSeparation = 6f;
            centerPoint.SetActive(true);
        }

        /// <summary>
        /// Called once per frame. Does not roll over to child classes
        /// </summary>
        private void Update()
        {
            CenterGridOnUpdate();
        }

        /// <summary>
        /// GlobalStartingProcedures
        ///
        /// Variables that need to be set for all tests under this class
        /// Think of it as a constructor.
        /// </summary>
        protected virtual void GlobalStartingProcedures()
        {
            blinkingLightsSeedForever = new List<BlinkingLight>();
            blinkingLightsActive = new List<BlinkingLight>();
            blinkingLightsRight = new List<BlinkingLight>();
            blinkingLightsLeft = new List<BlinkingLight>();
            blinkingLightsSeed = new List<BlinkingLight>();
            blinkingLightsUL = new List<BlinkingLight>();
            blinkingLightsUR = new List<BlinkingLight>();
            blinkingLightsBL = new List<BlinkingLight>();
            blinkingLightsBR = new List<BlinkingLight>();
            blinkingLights = new List<BlinkingLight>();
            bLightOutlier = new List<BlinkingLight>();
            BlinkingBoxes = new List<BlinkingLight>();

            lightsAndListsCoroutineComplete = false;
            ListsClearedAndLightsRemoved = false;
            coroutineComplete = false;
            isCompleted = false;

            FinalCenterPos = centerPoint.transform.position;
            CenterPosition.Enqueue(FinalCenterPos);
            CenterPosition = new Queue<Vector3>();
            lightIndex = -1;

        }

        /// <summary>
        /// CenterGridOnUpdate
        ///
        /// When Moving the grid, this needs to run ever update to properly keep the grid centered
        /// </summary>
        protected virtual void CenterGridOnUpdate()
        {
            if (CenterPointCentered)
            {
                if (MovementSetToQuad)
                {
                    MoveGridLocation(QuadBeingCentered);
                }
                else
                {
                    MoveGridLocation();
                }

                float TransSpeed = Time.deltaTime * GraphSpeed;

                if (!m_TestPaused)
                {
                    centerPoint.transform.position = Vector3.SlerpUnclamped(centerPoint.transform.position, FinalCenterPos, TransSpeed);
                }
                else
                {
                    TransSpeed = Time.deltaTime * 2.5f;
                    centerPoint.transform.position = Vector3.SlerpUnclamped(centerPoint.transform.position, FinalCenterPos, TransSpeed);
                }
            }
        }

        /// <summary>
        /// SetLightLocationsAndCreateLists
        ///
        /// Places lights in correct locations based on quadrand and sets their sections from center -> outward
        ///
        /// This also offloads lists that are needed after the start from loading when the app calls the test.
        /// This is designed to spread the processing over 4 frames rather than all at once.
        /// Will be useful later when the quadrants need specific DB for each light
        ///
        /// TODO: CREATE A LOADING SCREEN TO FULLY USE THIS
        ///
        /// </summary>
        public virtual IEnumerator SetLightLocationsAndCreateLists(EyeTestChoice eyeTestChoice)
        {
            testChoice = eyeTestChoice;
            if (blinkingLights != null)
            {
                ClearListsAndRemoveAllLights();
                yield return new WaitUntil(() => ListsClearedAndLightsRemoved);
            }
            else
            {
                Start();
            }

            ListsClearedAndLightsRemoved = false;
            Debug.Log("testChoice = " + testChoice.ToString());

            SetupCenterpointAndEyepoint();
            QuadBeingCentered = Quadrant.Center;
            centerPoint.transform.rotation = lCamera.transform.localRotation;
            yield return new WaitForSeconds(1f);

            if (BoxesActive)
            {
                switch (testChoice)
                {
                    case EyeTestChoice.Left:
                        BuildLeftEyeBox();
                        break;

                    case EyeTestChoice.Right:
                        BuildRightEyeBox();
                        break;

                    case EyeTestChoice.Both:
                        BuildLeftEyeBox();
                        BuildRightEyeBox();
                        break;

                    case EyeTestChoice.BinocularSingle:
                        Debug.Log("Shouldn't Get Here");
                        break;

                    case EyeTestChoice.None:
                        Debug.Log("Shouldn't Get Here");
                        break;

                    default:
                        break;
                }
            }
            else
            {
                switch (testChoice)
                {
                    case EyeTestChoice.Left:
                        BuildLeftEye();
                        break;

                    case EyeTestChoice.Right:
                        BuildRightEye();
                        break;

                    case EyeTestChoice.Both:
                        BuildLeftEye();
                        BuildRightEye();
                        break;

                    case EyeTestChoice.BinocularSingle:
                        Debug.LogError("Shouldn't Get Here");
                        break;

                    case EyeTestChoice.None:
                        Debug.LogError("Shouldn't Get Here");
                        break;

                    default:
                        break;
                }
            }
            lightsAndListsCoroutineComplete = true;
            
        }

        /// <summary>
        /// BuildRightEye:
        ///
        /// Creates all of the lights for the right eye
        /// </summary>
        /// <returns></returns>
        protected virtual void BuildRightEye()
        {
            Debug.Log("Building Right Eye");

            BuildLightsQuad(Quadrant.UL, CurrentEye.Right, out blinkingLightsUL);
            BuildLightsQuad(Quadrant.UR, CurrentEye.Right, out blinkingLightsUR);
            BuildLightsQuad(Quadrant.BL, CurrentEye.Right, out blinkingLightsBL);
            BuildLightsQuad(Quadrant.BR, CurrentEye.Right, out blinkingLightsBR);
            SetupOutlier(CurrentEye.Right, Quadrant.UL);
            SetupOutlier(CurrentEye.Right, Quadrant.BL);

            coroutineComplete = true;
            Debug.Log("Everything Right Built");
        }

        /// <summary>
        /// BuildLeftEye:
        ///
        /// Creates all of the lights for the left eye
        /// </summary>
        /// <returns></returns>
        protected virtual void BuildLeftEye()
        {
            Debug.Log("Building Left Eye");

            BuildLightsQuad(Quadrant.UL, CurrentEye.Left, out blinkingLightsUL);
            BuildLightsQuad(Quadrant.UR, CurrentEye.Left, out blinkingLightsUR);
            BuildLightsQuad(Quadrant.BL, CurrentEye.Left, out blinkingLightsBL);
            BuildLightsQuad(Quadrant.BR, CurrentEye.Left, out blinkingLightsBR);
            SetupOutlier(CurrentEye.Left, Quadrant.UR);
            SetupOutlier(CurrentEye.Left, Quadrant.BR);

            coroutineComplete = true;
            Debug.Log("Everything Left Built");
        }

        /// <summary>
        /// BuildRightEyeBox:
        ///
        /// Creates all of the Rectangles for the Right eye
        /// </summary>
        protected virtual void BuildRightEyeBox()
        {
            Debug.Log("Building Right Eye Box");

            AddNewBox(CurrentEye.Right, Quadrant.UL);
            AddNewBox(CurrentEye.Right, Quadrant.UR);
            AddNewBox(CurrentEye.Right, Quadrant.BL);
            AddNewBox(CurrentEye.Right, Quadrant.BR);

            coroutineComplete = true;
            Debug.Log("RightBoxBuilt");
        }

        /// <summary>
        /// BuildLeftEyeBox:
        ///
        /// Creates all of the Rectangles for the left eye
        /// </summary>
        protected virtual void BuildLeftEyeBox()
        {
            Debug.Log("Building Left Eye Box");

            AddNewBox(CurrentEye.Left, Quadrant.UL);
            AddNewBox(CurrentEye.Left, Quadrant.UR);
            AddNewBox(CurrentEye.Left, Quadrant.BL);
            AddNewBox(CurrentEye.Left, Quadrant.BR);

            coroutineComplete = true;
            Debug.Log("LeftBoxBuilt");
        }

        /// <summary>
        /// SetupCenterpointAndEyepoint:
        ///
        /// configures scale of the Centerpoint end collision of the eye point
        /// </summary>
        protected virtual void SetupCenterpointAndEyepoint()
        {
            float LightSize;
            float newScale;
            switch (MenuCanvas.Instance.testOptions.lightSize)
            {
                case 1:
                    LightSize = MoveToEye.instance.Distance * (1f / 660f);
                    break;

                case 3:
                default:
                    LightSize = MoveToEye.instance.Distance * (1f / 165f);
                    break;

                case 5:
                    LightSize = MoveToEye.instance.Distance * (4f / 165f);
                    break;
            }
            newScale = LightSize;

            Vector3 position = new Vector3(((lCamera.transform.position.x + rCamera.transform.position.x) / 2), ((lCamera.transform.position.y + rCamera.transform.position.y) / 2), ((lCamera.transform.position.z + rCamera.transform.position.z) / 2));
            VisCross.GetComponent<MeshRenderer>().enabled = false;
            if (centerPoint != null)
            {
                centerPoint.transform.localScale = new Vector3(newScale * 2f, newScale * 2f, newScale * 2f);

                Ray rayR1 = new Ray(position, ((lCamera.transform.forward + rCamera.transform.forward) / 2));
                RaycastHit raycastR1;
                Physics.Raycast(rayR1, out raycastR1, MoveToEye.instance.Distance + 1f, LAYER_LEFT);

                FinalCenterPos = raycastR1.point;
                centerPoint.transform.position = raycastR1.point;
                centerPoint.GetComponent<MeshRenderer>().enabled = true;
            }
            if (EyePoint != null)
            {
                EyePoint.transform.localScale = new Vector3(newScale, newScale, newScale);
            }
        }


        /// <summary>
        /// SetupOutliers:
        ///
        /// Adds the extra lights to the left or right eye
        /// </summary>
        /// <param name="currentEye">
        /// The left or right eye
        /// </param>
        /// <param name="_quad">
        /// The quad the outlier is located in. 
        /// </param>
        protected virtual void SetupOutlier(CurrentEye currentEye, Quadrant _quad)
        {
            GameObject testGO;
            RaycastHit raycast;
            Transform newTransformPos;
            Ray ray;
            BlinkingLight blight;
            Vector3 position;
            Vector3 scale;
            Quaternion rotation;

            float LightSize;
            float newScale;
            switch (MenuCanvas.Instance.testOptions.lightSize)
            {
                case 1:
                    LightSize = MoveToEye.instance.Distance * (1f / 660f);
                    break;

                case 3:
                default:
                    LightSize = MoveToEye.instance.Distance * (1f / 165f);
                    break;

                case 5:
                    LightSize = MoveToEye.instance.Distance * (4f / 165f);
                    break;
            }
            newScale = LightSize;


            //Create the correct test point and grab the BlinkingLight script from the instantiated object
            if (currentEye == CurrentEye.Left)
            {
                GameObject lOb = Instantiate(blinkingLightLeftPrefab);

                lightObjects.Add(lOb);
                blight = GetBlinkingLight(ref lOb);
            }
            else
            {
                GameObject lOb = Instantiate(blinkingLightRightPrefab);

                lightObjects.Add(lOb);
                blight = GetBlinkingLight(ref lOb);
            }

            blight.particle = particle;
            blight.sectionFromCenter = SectionFromCenter.Outlier;

            testGO = new GameObject();
            newTransformPos = testGO.transform;

            //Get the transform from the correct camera to gain the starting point for the test point.
            if (currentEye == CurrentEye.Left)
            {
                position = new Vector3(lCamera.transform.position.x, lCamera.transform.position.y, lCamera.transform.position.z);
                scale = new Vector3(lCamera.transform.localScale.x, lCamera.transform.localScale.y, lCamera.transform.localScale.z);
                rotation = new Quaternion(lCamera.transform.rotation.x, lCamera.transform.rotation.y, lCamera.transform.rotation.z, lCamera.transform.rotation.w);
            }
            else
            {
                position = new Vector3(rCamera.transform.position.x, rCamera.transform.position.y, rCamera.transform.position.z);
                scale = new Vector3(rCamera.transform.localScale.x, rCamera.transform.localScale.y, rCamera.transform.localScale.z);
                rotation = new Quaternion(rCamera.transform.rotation.x, rCamera.transform.rotation.y, rCamera.transform.rotation.z, rCamera.transform.rotation.w);
            }

            blight.x = gridSize + 1;
            blight.y = 0;

            blight.quad = _quad;

            switch (currentEye)
            {
                case CurrentEye.Left:
                    blinkingLightsLeft.Add(blight);

                    newTransformPos.position = new Vector3(position.x, position.y, position.z);
                    newTransformPos.localScale = new Vector3(scale.x, scale.y, scale.z);
                    newTransformPos.rotation = new Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);

                    if (_quad == Quadrant.UR)
                    {
                        blinkingLightsUR.Add(blight);

                        newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - blight.y * degreeOfSeparation),
                            (StartingDegreeOutward + blight.x * degreeOfSeparation), 0));
                    }
                    else if (_quad == Quadrant.BR)
                    {
                        blinkingLightsBR.Add(blight);

                        newTransformPos.Rotate(new Vector3((StartingDegreeOutward + blight.y * degreeOfSeparation),
                        (StartingDegreeOutward + blight.x * degreeOfSeparation), 0));

                    }
                    else
                    {
                        Debug.LogError("WrongQuadSet");
                    }
                    ray = new Ray(newTransformPos.position, newTransformPos.forward);
                    Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_LEFT);

                    blight.transform.position = raycast.point;
                    blight.transform.localScale = new Vector3(newScale, newScale, newScale);
                    blight.transform.rotation = Quaternion.LookRotation(raycast.point - position);

                    Destroy(testGO);
                    break;

                case CurrentEye.Right:
                    blinkingLightsRight.Add(blight);

                    testGO = new GameObject();
                    newTransformPos = testGO.transform;
                    newTransformPos.position = new Vector3(position.x, position.y, position.z);
                    newTransformPos.localScale = new Vector3(scale.x, scale.y, scale.z);
                    newTransformPos.rotation = new Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);

                    if (_quad == Quadrant.UL)
                    {
                        blinkingLightsUL.Add(blight);

                        newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - blight.y * degreeOfSeparation),
                            (-StartingDegreeOutward - blight.x * degreeOfSeparation), 0));
                    }
                    else if (_quad == Quadrant.BL)
                    {
                        blinkingLightsBL.Add(blight);

                        newTransformPos.Rotate(new Vector3((StartingDegreeOutward + blight.y * degreeOfSeparation),
                       (-StartingDegreeOutward - blight.x * degreeOfSeparation), 0));
                    }
                    else
                    {
                        Debug.LogError("WrongQuadSet");
                    }


                    ray = new Ray(newTransformPos.position, newTransformPos.forward);
                    Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_RIGHT);
                    blight.transform.position = raycast.point;
                    blight.transform.localScale = new Vector3(newScale, newScale, newScale);
                    blight.transform.rotation = Quaternion.LookRotation(raycast.point - position);

                    Destroy(testGO);
                    break;

                default:
                    break;
            }

            blinkingLights.Add(blight);
            bLightOutlier.Add(blight);

            blight.currentEye = currentEye;

            SetParent(blight.transform, centerPoint.transform);
        }



        /// <summary>
        /// BuildLightsQuad:
        /// Creates the lights for the given quad
        /// </summary>
        /// <param name="currentQuad"> The current quadrant to build for </param>
        /// <param name="currentEye">  The current eye to build for  </param>
        /// <param name="currentQuadLightList"> The List to add the lights to </param>
        protected virtual void BuildLightsQuad(Quadrant currentQuad, CurrentEye currentEye, out List<BlinkingLight> currentQuadLightList)
        {
            //Where location should be placed on the grid
            //X/Y: grid location going from center out based on quadrant
            int currentX, currentY;
            currentX = currentY = 0;
            int currentGridsize = gridSize;
            currentQuadLightList = new List<BlinkingLight>();
            bool firstRowComplete = false;
            for (int i = 0; i < quadLightCount; i++)
            {
                BlinkingLight _bl = AddNewLight(currentEye, currentQuad, currentX, currentY);
                if (_bl != null)
                {
                    _bl.x = currentX;
                    _bl.y = currentY;
                }

                if (currentX == 0)
                {
                    currentX++;
                }
                else if (currentX >= currentGridsize)
                {
                    currentY++;
                    currentX = 0;
                    if (firstRowComplete)
                    {
                        currentGridsize--;
                    }
                    else
                    {
                        firstRowComplete = true;
                    }
                }
                else
                {
                    currentX++;
                }


            }
        }

        /// <summary>
        /// Set lights around point that should be tested more.
        ///
        /// PREMATURE FUNCTION THAT IS CURRENTLY NOT NECESSARY
        /// TODO
        /// </summary>
        /// <param name="LightToSurround"></param>
        public void TestAroundLightPoint(BlinkingLight LightToSurround)
        {
            float newLightDistance = .5f / LightToSurround.CurrentLevel;

            TestAroundLightPointHelper(LightToSurround, LightToSurround.x - newLightDistance, LightToSurround.y);
            TestAroundLightPointHelper(LightToSurround, LightToSurround.x + newLightDistance, LightToSurround.y);

            TestAroundLightPointHelper(LightToSurround, LightToSurround.x, LightToSurround.y - newLightDistance);
            TestAroundLightPointHelper(LightToSurround, LightToSurround.x, LightToSurround.y + newLightDistance);

            TestAroundLightPointHelper(LightToSurround, LightToSurround.x + newLightDistance, LightToSurround.y - newLightDistance);
            TestAroundLightPointHelper(LightToSurround, LightToSurround.x - newLightDistance, LightToSurround.y + newLightDistance);

            TestAroundLightPointHelper(LightToSurround, LightToSurround.x + newLightDistance, LightToSurround.y + newLightDistance);
            TestAroundLightPointHelper(LightToSurround, LightToSurround.x - newLightDistance, LightToSurround.y - newLightDistance);
        }

        /// <summary>
        /// Sets light loication at designated point during test.
        /// Also changes current level of light. If level is higher than designated, light should not be created.
        /// TODO: add the second part of this feature
        /// </summary>
        /// <param name="LightToSurround"></param>
        /// <param name="LightDistanceX"></param>
        /// <param name="LightDistanceY"></param>
        private void TestAroundLightPointHelper(BlinkingLight LightToSurround, float LightDistanceX, float LightDistanceY)
        {
            BlinkingLight _curLight = AddNewLight(LightToSurround.currentEye, LightToSurround.quad, LightDistanceX, LightDistanceY, true);
            _curLight.CurrentLevel = LightToSurround.CurrentLevel + 1;
            _curLight.sectionFromCenter = LightToSurround.sectionFromCenter;
        }

        /// ADDNEWLIGHT
        /// <summary>
        /// Creates a test point in given location in the given quad
        /// </summary>
        /// <param name="currentEye">The eye the light should be created for.</param>
        /// <param name="_quad">The quadrant the light should be created.</param>
        /// <param name="x">How far over horizontally the light should be created.</param>
        /// <param name="y">How far over Vertically the light should be created.</param>
        /// <param name="duringTest">If light is being created while the test is running.</param>
        /// <returns>Returns the created light so it can be modified further</returns>
        public virtual BlinkingLight AddNewLight(CurrentEye currentEye, Quadrant _quad, float x, float y, bool duringTest = false)
        {
            Vector3 position;
            Vector3 scale;
            Quaternion rotation;
            float LightSize;
            float newScale;
            switch (MenuCanvas.Instance.testOptions.lightSize)
            {
                case 1:
                    LightSize = MoveToEye.instance.Distance * (1f / 660f);
                    break;

                case 3:
                default:
                    LightSize = MoveToEye.instance.Distance * (1f / 165f);
                    break;

                case 5:
                    LightSize = MoveToEye.instance.Distance * (4f / 165f);
                    break;
            }
            newScale = LightSize;

            GameObject testGO;
            Transform newTransformPos;
            RaycastHit raycast;
            Ray ray;
            BlinkingLight _bl;
            GameObject blGO;

            if (currentEye == CurrentEye.Left)
            {
                position = lCamera.transform.position;
                scale = lCamera.transform.localScale;
                rotation = lCamera.transform.rotation;
                blGO = Instantiate(blinkingLightLeftPrefab);
                _bl = GetBlinkingLight(ref blGO);
                blinkingLightsLeft.Add(_bl);
            }
            else
            {
                position = rCamera.transform.position;
                scale = rCamera.transform.localScale;
                rotation = rCamera.transform.rotation;
                blGO = Instantiate(blinkingLightRightPrefab);
                _bl = GetBlinkingLight(ref blGO);
                blinkingLightsRight.Add(_bl);
            }

            _bl.quad = _quad;
            _bl.particle = particle;
            _bl.currentEye = currentEye;
            lightObjects.Add(blGO);
            blinkingLights.Add(_bl);

            testGO = new GameObject();
            //create transform that will be set from the center of the camera
            newTransformPos = testGO.transform;
            newTransformPos.position = position;
            newTransformPos.localScale = scale;
            newTransformPos.rotation = rotation;

            switch (_bl.quad)
            {
                case Quadrant.UL:
                    newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - y * degreeOfSeparation), (-StartingDegreeOutward - x * degreeOfSeparation), 0));
                    blinkingLightsUL.Add(_bl);

                    break;

                case Quadrant.BL:
                    newTransformPos.Rotate(new Vector3((StartingDegreeOutward + y * degreeOfSeparation), (-StartingDegreeOutward - x * degreeOfSeparation), 0));
                    blinkingLightsBL.Add(_bl);

                    break;

                case Quadrant.UR:
                    newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - y * degreeOfSeparation), (StartingDegreeOutward + x * degreeOfSeparation), 0));
                    blinkingLightsUR.Add(_bl);

                    break;

                case Quadrant.BR:
                    newTransformPos.Rotate(new Vector3((StartingDegreeOutward + y * degreeOfSeparation), (StartingDegreeOutward + x * degreeOfSeparation), 0));
                    blinkingLightsBR.Add(_bl);

                    break;

                default:
                    break;
            }

            ray = new Ray(newTransformPos.position, newTransformPos.forward);

            if (currentEye == CurrentEye.Left)
            {
                Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_LEFT);
            }
            else
            {
                Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_RIGHT);
            }

            _bl.transform.position = raycast.point;
            _bl.transform.rotation = Quaternion.LookRotation(raycast.point - position);
            _bl.transform.localScale = new Vector3(newScale, newScale, newScale);

            Destroy(testGO);

            if (!duringTest)
            {
                if (x == 1 && y == 1)
                {
                    _bl.sectionFromCenter = SectionFromCenter.Seed;
                    blinkingLightsSeed.Add(_bl);
                    blinkingLightsSeedForever.Add(_bl);
                    blinkingLightsActive.Add(_bl);
                }
                else if (x <= 1 && y <= 1)
                {
                    _bl.sectionFromCenter = SectionFromCenter.Closer;
                }
                else
                {
                    _bl.sectionFromCenter = SectionFromCenter.Outer;
                }
            }
            SetParent(blGO.transform, centerPoint.transform);

            return _bl;
        }

        /// AddNewBox
        /// <summary>
        /// Creates a Box Test Point in the given quad
        /// </summary>
        /// <param name="currentEye">The eye the light should be created for.</param>
        /// <param name="_quad">The quadrant the light should be created.</param>
        /// <returns>Returns the created light so it can be modified further</returns>
        public virtual BlinkingLight AddNewBox(CurrentEye currentEye, Quadrant _quad)
        {
            Vector3 position;
            Vector3 scale;
            Quaternion rotation;
            GameObject testGO;
            RaycastHit raycast;
            Transform newTransformPos;
            Ray ray;
            BlinkingLight _bl;
            GameObject blGO;

            float newScaleWide = GetScaleDegrees(14f);
            float newScaleTall = GetScaleDegrees(9f);
            float LightSize;
            float newScale;
            switch (MenuCanvas.Instance.testOptions.lightSize)
            {
                case 1:
                    LightSize = MoveToEye.instance.Distance * (1f / 660f);
                    break;

                case 3:
                default:
                    LightSize = MoveToEye.instance.Distance * (1f / 165f);
                    break;

                case 5:
                    LightSize = MoveToEye.instance.Distance * (4f / 165f);
                    break;
            }
            newScale = LightSize;

            float DegreeOutwardX = 13f, DegreeOutwardY = 10.5f;

            if (currentEye == CurrentEye.Left)
            {
                position = lCamera.transform.position;
                scale = lCamera.transform.localScale;
                rotation = lCamera.transform.rotation;
                blGO = Instantiate(blinkingBoxLeftPrefab);
                _bl = GetBlinkingLight(ref blGO);
                blinkingLightsLeft.Add(_bl);
            }
            else
            {
                position = rCamera.transform.position;
                scale = rCamera.transform.localScale;
                rotation = rCamera.transform.rotation;
                blGO = Instantiate(blinkingBoxRightPrefab);
                _bl = GetBlinkingLight(ref blGO);
                blinkingLightsRight.Add(_bl);
            }

            _bl.quad = _quad;
            _bl.particle = particle;
            _bl.currentEye = currentEye;
            m_BoxObjects.Add(blGO);
            blinkingLights.Add(_bl);

            testGO = new GameObject();

            //create transform that will be set from the center of the camera
            newTransformPos = testGO.transform;
            newTransformPos.position = position;
            newTransformPos.localScale = scale;
            newTransformPos.rotation = rotation;

            switch (_bl.quad)
            {
                case Quadrant.UL:
                    newTransformPos.Rotate(new Vector3(-DegreeOutwardY, -DegreeOutwardX, 0));
                    blinkingLightsUL.Add(_bl);

                    break;

                case Quadrant.BL:
                    newTransformPos.Rotate(new Vector3(DegreeOutwardY, -DegreeOutwardX, 0));
                    blinkingLightsBL.Add(_bl);

                    break;

                case Quadrant.UR:
                    newTransformPos.Rotate(new Vector3(-DegreeOutwardY, DegreeOutwardX, 0));
                    blinkingLightsUR.Add(_bl);

                    break;

                case Quadrant.BR:
                    newTransformPos.Rotate(new Vector3(DegreeOutwardY, DegreeOutwardX, 0));
                    blinkingLightsBR.Add(_bl);

                    break;

                default:
                    break;
            }

            ray = new Ray(newTransformPos.position, newTransformPos.forward);

            if (currentEye == CurrentEye.Left)
            {
                Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_LEFT);
            }
            else
            {
                Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_RIGHT);
            }

            _bl.transform.position = raycast.point;
            _bl.transform.rotation = Quaternion.LookRotation(raycast.point - position, rCamera.transform.up);
            _bl.transform.localScale = new Vector3(newScaleWide, newScaleTall, newScale);

            Destroy(testGO);

            _bl.sectionFromCenter = SectionFromCenter.Box;
            BlinkingBoxes.Add(_bl);
            BlinkingBoxesActive.Add(_bl);

            SetParent(blGO.transform, centerPoint.transform);

            return _bl;
        }

        /// <summary>
        /// Takes in the degrees that the that the value should be and will use the current distance to figure out the size it should be.
        /// </summary>
        /// <param name="Degrees"></param>
        /// <returns></returns>
        public virtual float GetScaleDegrees(float degs)
        {
            return Mathf.Tan(degs * Mathf.PI / 180f) * MoveToEye.instance.Distance;
        }

        /// <summary>
        /// Sets light location for lights that are not bound by a local grid.
        /// </summary>
        /// <param name="_light">Light to be placed</param>
        public virtual void SetLightLocation(ref BlinkingLight _light)
        {
            Vector3 position;
            Vector3 scale;
            float LightSize;
            float newScale;
            switch (MenuCanvas.Instance.testOptions.lightSize)
            {
                case 1:
                    LightSize = MoveToEye.instance.Distance * (1f / 660f);
                    break;

                case 3:
                default:
                    LightSize = MoveToEye.instance.Distance * (1f / 165f);
                    break;

                case 5:
                    LightSize = MoveToEye.instance.Distance * (4f / 165f);
                    break;
            }
            newScale = LightSize;

            Transform _parent = _light.transform.parent;

            _light.transform.SetParent(null);

            if (_light.currentEye == CurrentEye.Left)
            {
                position = new Vector3(lCamera.transform.position.x, lCamera.transform.position.y, lCamera.transform.position.z);
                scale = new Vector3(lCamera.transform.localScale.x, lCamera.transform.localScale.y, lCamera.transform.localScale.z);
            }
            else
            {
                position = new Vector3(rCamera.transform.position.x, rCamera.transform.position.y, rCamera.transform.position.z);
                scale = new Vector3(rCamera.transform.localScale.x, rCamera.transform.localScale.y, rCamera.transform.localScale.z);
            }

            GameObject testGO;
            RaycastHit raycast;
            Transform newTransformPos;
            Ray ray;

            testGO = new GameObject();
            //create transform that will be set from the center of the camera
            newTransformPos = testGO.transform;
            newTransformPos.position = new Vector3(position.x, position.y, position.z);
            newTransformPos.localScale = new Vector3(scale.x, scale.y, scale.z);
            newTransformPos.rotation = Quaternion.LookRotation(centerPoint.transform.position - position);// new Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);

            switch (_light.quad)
            {
                case Quadrant.UL:
                    newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - _light.y * degreeOfSeparation), (-StartingDegreeOutward - _light.x * degreeOfSeparation), 0));
                    break;

                case Quadrant.BL:
                    newTransformPos.Rotate(new Vector3((StartingDegreeOutward + _light.y * degreeOfSeparation), (-StartingDegreeOutward - _light.x * degreeOfSeparation), 0));
                    break;

                case Quadrant.UR:
                    newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - _light.y * degreeOfSeparation), (StartingDegreeOutward + _light.x * degreeOfSeparation), 0));
                    break;

                case Quadrant.BR:
                    newTransformPos.Rotate(new Vector3((StartingDegreeOutward + _light.y * degreeOfSeparation), (StartingDegreeOutward + _light.x * degreeOfSeparation), 0));
                    break;

                default:
                    break;
            }

            ray = new Ray(newTransformPos.position, newTransformPos.forward);

            if (_light.currentEye == CurrentEye.Left)
            {
                Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_LEFT);
            }
            else
            {
                Physics.Raycast(ray, out raycast, MoveToEye.instance.Distance + 1f, LAYER_RIGHT);
            }

            _light.transform.position = raycast.point;
            _light.transform.rotation = Quaternion.LookRotation(raycast.point - position);
            _light.transform.localScale = new Vector3(newScale, newScale, newScale);
            _light.transform.SetParent(_parent);

            Destroy(testGO);
        }

        /// <summary>
        /// Sets the object in the local space of the second object
        /// </summary>
        protected virtual void SetParent(Transform child, Transform parent)
        {
            child.transform.SetParent(parent.transform, true);
        }

        /// <summary>
        /// Get the light from the object that contains it.
        /// </summary>
        /// <param name="_blight">
        /// The object containing a light
        /// </param>
        protected virtual BlinkingLight GetBlinkingLight(ref GameObject _blight)
        {
            return _blight.GetComponent<BlinkingLight>();
        }

        /// <summary>
        /// SetDBOnRestOfLights
        ///
        /// Is called when the first set of seed lights have been completed.
        /// Will set the rest of the quadrant lights to their designated DB
        /// </summary>
        protected virtual void SetDBOnRestOfLights()
        {
            Debug.Log("SetDBOnRestOfLights");

            foreach (var item in blinkingLightsSeedForever)
            {
                switch (item.quad)
                {
                    case Quadrant.UL:
                        SetDBQuad(item, blinkingLightsUL);
                        break;

                    case Quadrant.BL:
                        SetDBQuad(item, blinkingLightsBL);
                        break;

                    case Quadrant.UR:
                        SetDBQuad(item, blinkingLightsUR);
                        break;

                    case Quadrant.BR:
                        SetDBQuad(item, blinkingLightsBR);
                        break;

                    default:
                        break;
                }
            }
        }

        /// <summary>
        /// SetDBOnRestOfQuad
        ///
        /// Is called when the first set of seed lights have been completed.
        /// Will set the rest of the quadrant lights to their designated DB
        /// </summary>
        protected virtual void SetDBOnRestOfQuad(BlinkingLight SeedNode)
        {
            switch (SeedNode.quad)
            {
                case Quadrant.UL:
                    SetDBQuad(SeedNode, blinkingLightsUL);
                    break;

                case Quadrant.BL:
                    SetDBQuad(SeedNode, blinkingLightsBL);
                    break;

                case Quadrant.UR:
                    SetDBQuad(SeedNode, blinkingLightsUR);
                    break;

                case Quadrant.BR:
                    SetDBQuad(SeedNode, blinkingLightsBR);
                    break;

                default:
                    break;
            }
        }

        /// <summary>
        /// SetDBQuad:
        ///
        /// Sets the DB for the rest of the lights within the given quad
        /// </summary>
        /// <param name="seedLight">
        /// Given seed light that has been completed
        /// </param>
        /// <param name="lights">
        /// The lights from the given quad
        /// </param>
        protected virtual void SetDBQuad(BlinkingLight seedLight, List<BlinkingLight> lights)
        {
            //list for UL Quad
            foreach (var item in lights)
            {
                switch (item.sectionFromCenter)
                {
                    case SectionFromCenter.Closer:
                        item.currentDB = seedLight.currentDB + 2;
                        blinkingLightsActive.Add(item);

                        break;

                    case SectionFromCenter.Outer:
                    case SectionFromCenter.Outlier:
                        item.currentDB = seedLight.currentDB - 2;
                        blinkingLightsActive.Add(item);

                        break;

                    default:
                        break;
                }
            }
        }

        /// <summary>
        /// ClearListsAndRemoveAllLights:
        ///
        /// Removes ALL lights and clears lists to start fresh
        /// </summary>
        public virtual void ClearListsAndRemoveAllLights()
        {
            bool lightsRemoved = false, listsCleared = true;
            RemoveAllLights(ref lightsRemoved);
            ClearLists(ref listsCleared);
            ListsClearedAndLightsRemoved = true;
        }

        /// <summary>
        /// ClearLists:
        ///
        /// Cleans out lists to be reused
        /// </summary>
        /// <param name="listsCleared">
        /// reference to bool letting called fuction know that this coroutine is complete.
        /// </param>
        /// <returns></returns>
        protected virtual void ClearLists(ref bool listsCleared)
        {
            bLightOutlier.Clear();
            blinkingLightsUL.Clear();
            blinkingLightsUR.Clear();
            blinkingLightsBL.Clear();
            blinkingLightsBR.Clear();
            blinkingLightsSeed.Clear();
            blinkingLightsLeft.Clear();
            blinkingLightsRight.Clear();
            blinkingLightsActive.Clear();
            blinkingLightsSeedForever.Clear();

            listsCleared = true;
        }

        /// <summary>
        /// RemoveAllLights:
        ///
        /// Clears lights from scene
        /// </summary>
        /// <param name="lightsCleared">
        /// reference to bool letting called fuction know that this coroutine is complete.
        /// </param>
        /// <returns></returns>
        protected virtual void RemoveAllLights(ref bool lightsCleared)
        {
            foreach (var item in blinkingLights)
            {
                Destroy(item.GetComponent<GameObject>());
            }
            blinkingLights.Clear();
            lightsCleared = true;
        }

        /// <summary>
        /// StartTest:
        ///
        /// Starts Chosen Test
        /// </summary>
        public virtual IEnumerator StartTest()
        {
            Debug.Log("StartingTest");
            MoveGridLocation(Quadrant.Below);
            QuadBeingCentered = Quadrant.Below;
            yield return new WaitForSeconds(2);

            StartCoroutine(UFOScript.instance.MoveToCrosshair(QuadBeingCentered));
            yield return new WaitUntil(() => UFOScript.instance.moved);
            VisCross.GetComponent<MeshRenderer>().enabled = true;

            QuadBeingCentered = Quadrant.Center;
            m_TestPaused = true;
            yield return new WaitForSeconds(3f);

            StartCoroutine(UFOScript.instance.MoveOffScreen(QuadBeingCentered));
            yield return new WaitUntil(() => UFOScript.instance.moved);
            m_TestPaused = false;

            //start countdown
            yield return new WaitForSeconds(1f);
            UFOScript.instance.particles.Stop();

            if (BoxesActive)
            {
                while (BlinkingBoxesActive.Count > 0)
                {
                    if (eyeManager != null && eyeManager.IsBlinking() && pausingActivated)
                    {
                        if (!currentlyBlinking)
                        {
                            Debug.Log("blinking");
                            totalBlinks++;
                            currentlyBlinking = true;
                        }
                        yield return null;
                    }
                    else if (currentlyBlinking)
                    {
                        //TODO add Pause notification
                        currentlyBlinking = false;
                        yield return new WaitForSeconds(0.2f);
                    }
                    else
                    {
                        if (CenterPointMovementActivated)
                        {
                            yield return new WaitForSeconds(.2f);
                        }

                        if (arrowPointer.IsStaringAtCross())
                        {
                            bool _falsePosCheck = false;

                            _falsePosCheck = RunAFalsePosCheck();

                            if (_falsePosCheck)
                            {
                                Debug.Log("Running False Positive" + "\nTime Start: " + Time.time);
                                RunFalsePositive();
                                yield return new WaitUntil(() => falsePositive.isActive == false);
                                Debug.Log("Time End: " + Time.time);
                            }
                            else
                            {
                                StartCoroutine(RunNextTest(BlinkingBoxesActive));
                                if (CenterPointMovementActivated)
                                {
                                    yield return new WaitUntil(() => TestStarted);
                                    if (LightsSeen.Count > 0)
                                    {
                                        yield return new WaitUntil(() => chosenLight.isActive == false);
                                    }
                                    else
                                    {
                                        yield return new WaitForSeconds(0.1f);
                                    }
                                }
                                else
                                {
                                    yield return new WaitForSeconds(1f);
                                }
                                Debug.Log("TESTBASE Time End: " + Time.time);

                                if (lightFound)
                                {
                                    //QuadBeingCentered = chosenLight.quad;

                                    if (chosenLight.LightHit && LightsSeen.Count > 0)
                                    {
                                        MoveGridLocation(chosenLight, true);
                                    }
                                    lightFound = false;

                                    TestStarted = false;
                                }
                                CleanList(chosenLight, true);
                            }
                        }
                    }
                }
                ///Build Lights Here
                ///TestRemainingQuadrants
                BuildQuadsFromBoxes();
            }

            while (blinkingLightsSeed.Count > 0)
            {
                if (eyeManager != null && eyeManager.IsBlinking() && pausingActivated)
                {
                    if (!currentlyBlinking)
                    {
                        Debug.Log("blinking");
                        totalBlinks++;
                        currentlyBlinking = true;
                    }
                    yield return null;
                }
                else if (currentlyBlinking)
                {
                    //TODO add Pause notification
                    currentlyBlinking = false;
                    yield return new WaitForSeconds(0.2f);
                }
                else
                {
                    if (CenterPointMovementActivated)
                    {
                        yield return new WaitForSeconds(.2f);
                    }
                    if (arrowPointer.IsStaringAtCross())
                    {
                        bool _falsePosCheck = false;

                        _falsePosCheck = RunAFalsePosCheck();

                        if (_falsePosCheck)
                        {
                            Debug.Log("Running False Positive" + "\nTime Start: " + Time.time);
                            RunFalsePositive();
                            yield return new WaitForSeconds(1f);
                            yield return new WaitUntil(() => falsePositive.isActive == false);
                            Debug.Log("Time End: " + Time.time);
                        }
                        else
                        {
                            StartCoroutine(RunNextTest(blinkingLightsActive));
                            if (CenterPointMovementActivated)
                            {
                                yield return new WaitUntil(() => TestStarted);
                                if (LightsSeen.Count > 0)
                                {
                                    yield return new WaitUntil(() => chosenLight.isActive == false);
                                }
                                else
                                {
                                    yield return new WaitForSeconds(0.1f);
                                }
                            }
                            else
                            {
                                yield return new WaitForSeconds(1f);
                            }
                            Debug.Log("TESTBASE Time End: " + Time.time);

                            if (lightFound)
                            {
                                //QuadBeingCentered = chosenLight.quad;

                                if (chosenLight.LightHit && LightsSeen.Count > 0)
                                {
                                    MoveGridLocation(chosenLight, true);
                                }
                                lightFound = false;

                                TestStarted = false;
                            }
                            CleanList(chosenLight);
                        }
                    }
                }
            }

            //Set the rest of the lights up with the correct DB
            while (blinkingLightsActive.Count > 0)
            {
                if (eyeManager != null && eyeManager.IsBlinking() && pausingActivated)
                {
                    if (!currentlyBlinking)
                    {
                        totalBlinks++;
                        currentlyBlinking = true;
                    }
                    yield return null;
                }
                else if (currentlyBlinking)
                {
                    currentlyBlinking = false;
                    yield return new WaitForSeconds(0.5f);
                }
                else
                {
                    if (CenterPointMovementActivated)
                    {
                        yield return new WaitForSeconds(.2f);
                    }

                    if (arrowPointer.IsStaringAtCross())
                    {
                        int _falseTestCheck = 0;
                        _falseTestCheck = RunAFalseTestCheck();

                        if (_falseTestCheck == 1)
                        {
                            Debug.Log("Running False Negative" + "\nTime Start: " + Time.time);
                            RunFalseNegative();
                            yield return new WaitForSeconds(1f);
                            yield return new WaitUntil(() => falseNegative.isActive == false);
                            Debug.Log("Time End: " + Time.time);
                        }
                        else if (_falseTestCheck == 2)
                        {
                            Debug.Log("Running False Positive" + "\nTime Start: " + Time.time);
                            RunFalsePositive();
                            yield return new WaitForSeconds(1f);
                            yield return new WaitUntil(() => falsePositive.isActive == false);
                            Debug.Log("Time End: " + Time.time);
                        }
                        else
                        {
                            Debug.Log("RunNextTest");

                            StartCoroutine(RunNextTest(blinkingLightsActive));
                            if (CenterPointMovementActivated)
                            {
                                yield return new WaitUntil(() => TestStarted);

                                if (LightsSeen.Count > 0)
                                {
                                    yield return new WaitUntil(() => chosenLight.isActive == false);
                                }
                            }
                            else
                            {
                                yield return new WaitForSeconds(1f);
                            }

                            if (lightFound)
                            {
                                TestStarted = false;
                                if (chosenLight.LightHit && LightsSeen.Count > 0)
                                {
                                    MoveGridLocation(chosenLight, true);
                                }
                                lightFound = false;
                            }
                            CleanList(chosenLight);
                        }
                    }
                    else
                    {
                        yield return new WaitForSeconds(.1f);
                    }
                }
            }
            //run complete info here
            ShowResults();
        }

        /// <summary>
        /// BuildRemainingQuads:
        /// Takes the results of the rectangles and builds lights in seen quads.
        /// </summary>
        protected virtual void BuildQuadsFromBoxes()
        {
            foreach (var _box in BlinkingBoxes)
            {
                if (_box.highestDB > 0)
                {
                    switch (_box.quad)
                    {
                        case Quadrant.UL:
                            BuildLightsQuad(_box.quad, _box.currentEye, out blinkingLightsUL);
                            if (_box.currentEye == CurrentEye.Right)
                            {
                                SetupOutlier(CurrentEye.Right, Quadrant.UL);
                            }
                            break;

                        case Quadrant.BL:
                            BuildLightsQuad(_box.quad, _box.currentEye, out blinkingLightsBL);
                            if (_box.currentEye == CurrentEye.Right)
                            {
                                SetupOutlier(CurrentEye.Right, Quadrant.BL);
                            }
                            break;

                        case Quadrant.UR:
                            BuildLightsQuad(_box.quad, _box.currentEye, out blinkingLightsUR);
                            if (_box.currentEye == CurrentEye.Left)
                            {
                                SetupOutlier(CurrentEye.Left, Quadrant.UR);
                            }
                            break;

                        case Quadrant.BR:
                            BuildLightsQuad(_box.quad, _box.currentEye, out blinkingLightsBR);
                            if (_box.currentEye == CurrentEye.Left)
                            {
                                SetupOutlier(CurrentEye.Left, Quadrant.BR);
                            }
                            break;

                        case Quadrant.Center:
                            break;

                        case Quadrant.Below:
                            break;

                        default:
                            break;
                    }
                }
                _box.GetComponent<MeshRenderer>().enabled = false;

                //_box.GetComponent<MeshRenderer>().material.color = new Color(1,1,1,1);
            }
        }

        /// <summary>
        /// RunAFalseTestCheck:
        ///
        /// Check to see if a false positive or negative will run during next test
        /// </summary>
        protected virtual int RunAFalseTestCheck()
        {
            if (FalsePosNegTestsActive)
            {
                var num = Random.Range(0, 10);
                if (num > 5)
                {
                    //first check is for False Neg
                    if (Random.Range(0, 20) == 1)
                    {
                        return 1;
                    }
                    //second test is for false Pos
                    if (Random.Range(0, 70) == 1)
                    {
                        return 2;
                    }

                    return 0;
                }
                else
                {
                    //second test is for false Pos
                    if (Random.Range(0, 70) == 1)
                    {
                        return 2;
                    }
                    //first check is for False Neg
                    if (Random.Range(0, 20) == 1)
                    {
                        return 1;
                    }

                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }

        /// <summary>
        /// RunAFalsePosCheck:
        ///
        /// Check to see if a false positive will run during seed test
        /// </summary>
        protected virtual bool RunAFalsePosCheck()
        {
            if (FalsePosNegTestsActive)
                return (Random.Range(0, 1000) > 750);
            else
                return false;
        }

        /// <summary>
        /// RunFalsePositive:
        ///
        /// Doesn't blink for the same time a light would normally last
        /// </summary>
        protected virtual void RunFalsePositive()
        {
            falsePositive.FakeBlinkLight();
        }

        /// <summary>
        /// RunFalseNegative:
        ///
        /// Picks a seed light and makes it blink at 9db lower than current blinks
        /// </summary>
        protected virtual void RunFalseNegative()
        {
            if (blinkingLightsSeedForever.Count > 0)
            {
                lightIndex = Random.Range(0, blinkingLightsSeedForever.Count);
                falseNegative.BlinkBrighterLight(blinkingLightsSeedForever[lightIndex], !CenterPointMovementActivated);
            }
        }

        /// <summary>
        /// Takes a light that is active and finds light that is the opposite of it to test to
        /// move the grid into a correct location to hit a light that is still active.
        /// </summary>
        /// <param name="_currentRandomLight"></param>
        protected virtual void FindOppositeLightAndTestIt(BlinkingLight _currentRandomLight)
        {
            LightsSeen.Clear();
            LightsSeenSeed.Clear();
            LightsSeenOuter.Clear();
            LightsSeenOutlier.Clear();
            Plane[] planes = GeometryUtility.CalculateFrustumPlanes(viewCameraL);
            TestStarted = false;

            switch (_currentRandomLight.quad)
            {
                case Quadrant.UL:
                    foreach (var item in blinkingLightsBR)
                    {
                        if (GeometryUtility.TestPlanesAABB(planes, item.capsuleCollider.bounds) && item.complete)
                        {
                            LightsSeen.Add(item);

                            if (item.sectionFromCenter == SectionFromCenter.Seed)
                            {
                                LightsSeenSeed.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outlier)
                            {
                                LightsSeenOutlier.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outer)
                            {
                                LightsSeenOuter.Add(item);
                            }
                        }
                    }

                    break;

                case Quadrant.BL:
                    foreach (var item in blinkingLightsUR)
                    {
                        if (GeometryUtility.TestPlanesAABB(planes, item.capsuleCollider.bounds) && item.complete)
                        {
                            LightsSeen.Add(item);

                            if (item.sectionFromCenter == SectionFromCenter.Seed)
                            {
                                LightsSeenSeed.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outlier)
                            {
                                LightsSeenOutlier.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outer)
                            {
                                LightsSeenOuter.Add(item);
                            }
                        }
                    }

                    break;

                case Quadrant.UR:
                    foreach (var item in blinkingLightsBL)
                    {
                        if (GeometryUtility.TestPlanesAABB(planes, item.capsuleCollider.bounds) && item.complete)
                        {
                            LightsSeen.Add(item);

                            if (item.sectionFromCenter == SectionFromCenter.Seed)
                            {
                                LightsSeenSeed.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outlier)
                            {
                                LightsSeenOutlier.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outer)
                            {
                                LightsSeenOuter.Add(item);
                            }
                        }
                    }

                    break;

                case Quadrant.BR:
                    foreach (var item in blinkingLightsUL)
                    {
                        if (GeometryUtility.TestPlanesAABB(planes, item.capsuleCollider.bounds) && item.complete)
                        {
                            LightsSeen.Add(item);

                            if (item.sectionFromCenter == SectionFromCenter.Seed)
                            {
                                LightsSeenSeed.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outlier)
                            {
                                LightsSeenOutlier.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outer)
                            {
                                LightsSeenOuter.Add(item);
                            }
                        }
                    }

                    break;

                default:
                    break;
            }

            if (_currentRandomLight.sectionFromCenter != SectionFromCenter.Outlier)
            {
                foreach (var item in LightsSeen)
                {
                    if (item.x == _currentRandomLight.x && item.y == _currentRandomLight.y)
                    {
                        chosenLight = item;
                        item.BlinkMovingLight(!CenterPointMovementActivated);
                        TestStarted = true;
                        lightFound = true;
                        break;
                    }
                }

                if (!TestStarted)
                {
                    if (LightsSeenOuter.Count > 0)
                    {
                        chosenLight = LightsSeenOuter[Random.Range(0, LightsSeenOuter.Count)];
                        chosenLight.BlinkMovingLight(!CenterPointMovementActivated);
                        TestStarted = true;
                        lightFound = true;
                    }
                    else if (LightsSeenSeed.Count > 0)
                    {
                        chosenLight = LightsSeenSeed[Random.Range(0, LightsSeenSeed.Count)];
                        chosenLight.BlinkMovingLight(!CenterPointMovementActivated);
                        TestStarted = true;
                        lightFound = true;
                    }
                    else if (LightsSeen.Count > 0)
                    {
                        chosenLight = LightsSeen[Random.Range(0, LightsSeen.Count)];
                        chosenLight.BlinkMovingLight(!CenterPointMovementActivated);
                        TestStarted = true;
                        lightFound = true;
                    }
                    else
                    {
                        Debug.Log("LightsSeen not found");
                    }
                }
            }
            else
            {
                if (!TestStarted)
                {
                    if (LightsSeenOuter.Count > 0)
                    {
                        chosenLight = LightsSeenOuter[Random.Range(0, LightsSeenOuter.Count)];
                        chosenLight.BlinkMovingLight(!CenterPointMovementActivated);
                        TestStarted = true;
                        lightFound = true;
                    }
                    else if (LightsSeenSeed.Count > 0)
                    {
                        chosenLight = LightsSeenSeed[Random.Range(0, LightsSeenSeed.Count)];
                        chosenLight.BlinkMovingLight(!CenterPointMovementActivated);
                        TestStarted = true;
                        lightFound = true;
                    }
                    else if (LightsSeen.Count > 0)
                    {
                        chosenLight = LightsSeen[Random.Range(0, LightsSeen.Count)];
                        chosenLight.BlinkMovingLight(!CenterPointMovementActivated);
                        TestStarted = true;
                        lightFound = true;
                    }
                    else
                    {
                        Debug.Log("LightsSeen not found");
                    }
                }
            }
        }

        /// <summary>
        /// ShowResults:
        ///
        /// Once the test is completed, show the results
        /// This is more for demo purposes than it is for actual practice.
        /// </summary>
        protected virtual void ShowResults()
        {
            foreach (var item in blinkingLights)
            {
                //item.ShowResults();
                totalBlinks += item.blinkCount;
            }
            //MoveToEye.instance.gridMovesWithHead = false;
            LightFromCenterDiff = new Vector3();
            MovementSetToQuad = false;
            isCompleted = true;
            centerPoint.GetComponent<MeshRenderer>().enabled = false;
        }

        /// <summary>
        /// CleanList:
        ///
        /// Check to see if that blinking light is complete. if so, remove it from the active list.
        /// </summary>
        protected virtual bool CleanList(BlinkingLight light, bool _isBox = false)
        {
            if (light != null && (light.currentState == BlinkingLightState.Completed || light.complete))
            {
                Debug.Log(("Light completed: " + light.currentEye + " " + light.quad + " (" + light.x + ", " + light.y + ") Final DB: " + light.highestDB));
                if (_isBox)
                {
                    BlinkingBoxesActive.Remove(light);
                    GridMoved = true;
                    return true;
                }
                else
                {
                    blinkingLightsActive.Remove(light);
                    if (light.sectionFromCenter == SectionFromCenter.Seed)
                    {
                        blinkingLightsSeed.Remove(light);
                        SetDBOnRestOfQuad(light);

                    }
                    GridMoved = true;
                    return true;
                }
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// SuperCleanList:
        ///
        /// Check to see if that blinking light is complete. if so, remove it from the active list.
        /// </summary>
        protected virtual bool SuperCleanList()
        {
            List<BlinkingLight> listoLights = new List<BlinkingLight>(blinkingLightsActive);
            bool pointCleaned = false;
            foreach (var light in listoLights)
            {
                bool _p = CleanList(light);

                if (!pointCleaned && _p)
                {
                    pointCleaned = true;
                }
            }
            return pointCleaned;
        }

        /// <summary>
        /// RunNextTest:
        ///
        /// Randomly pick from list of active lights and make it blink
        /// </summary>
        protected virtual IEnumerator RunNextTest(List<BlinkingLight> currentList)
        {
            Debug.Log("RunNextTest");
            if (BlinkingBoxesActive.Count > 0)
            {
                LightsSeen.Clear();
                LightsSeen = new List<BlinkingLight>(BlinkingBoxesActive);
                chosenLight = null;
            }
            else
            {
                StartCoroutine(WhatCanBeSeen(currentList));
                yield return new WaitUntil(() => WhatCanBeSeenCompleted);
            }

            WhatCanBeSeenCompleted = false;

            //Set light to go off. Going from least to most important.
            //Boxes are checked for last but should be the only lights going off.
            if (LightsSeen.Count > 0 && (chosenLight == null || !chosenLight.isActive))
            {
                if (LightsSeenSeed.Count > 1)
                {
                    lightIndex = Random.Range(0, LightsSeenSeed.Count);
                    Debug.Log("CurrentEye: " + LightsSeenSeed[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeenSeed[lightIndex].x + ", " + LightsSeenSeed[lightIndex].y + "\ncurrentDB: " + LightsSeenSeed[lightIndex].currentDB + "\nTime Start: " + Time.time);
                    chosenLight = LightsSeenSeed[lightIndex];
                }
                else if (LightsSeenSeed.Count > 0)
                {
                    if (Random.Range(0, 100) > 75)
                    {
                        lightIndex = Random.Range(0, LightsSeen.Count);
                        Debug.Log("CurrentEye: " + LightsSeen[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeen[lightIndex].x + ", " + LightsSeen[lightIndex].y + "\ncurrentDB: " + LightsSeen[lightIndex].currentDB + "\nTime Start: " + Time.time);
                        chosenLight = LightsSeen[lightIndex];
                    }
                    else
                    {
                        lightIndex = Random.Range(0, LightsSeenSeed.Count);
                        Debug.Log("CurrentEye: " + LightsSeenSeed[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeenSeed[lightIndex].x + ", " + LightsSeenSeed[lightIndex].y + "\ncurrentDB: " + LightsSeenSeed[lightIndex].currentDB + "\nTime Start: " + Time.time);
                        chosenLight = LightsSeenSeed[lightIndex];
                    }
                }
                else if (LightsSeenOutlier.Count > 0)
                {
                    if (Random.Range(0, 100) > 75)
                    {
                        lightIndex = Random.Range(0, LightsSeenOutlier.Count);
                        Debug.Log("CurrentEye: " + LightsSeenOutlier[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeenOutlier[lightIndex].x + ", " + LightsSeenOutlier[lightIndex].y + "\ncurrentDB: " + LightsSeenOutlier[lightIndex].currentDB + "\nTime Start: " + Time.time);
                        chosenLight = LightsSeenOutlier[lightIndex];
                    }
                    else if (LightsSeenOuter.Count > 0)
                    {
                        if (Random.Range(0, 100) > 75)
                        {
                            lightIndex = Random.Range(0, LightsSeenOuter.Count);
                            Debug.Log("CurrentEye: " + LightsSeenOuter[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeenOuter[lightIndex].x + ", " + LightsSeenOuter[lightIndex].y + "\ncurrentDB: " + LightsSeenOuter[lightIndex].currentDB + "\nTime Start: " + Time.time);
                            chosenLight = LightsSeenOuter[lightIndex];
                        }
                        else
                        {
                            lightIndex = Random.Range(0, LightsSeen.Count);
                            Debug.Log("CurrentEye: " + LightsSeen[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeen[lightIndex].x + ", " + LightsSeen[lightIndex].y + "\ncurrentDB: " + LightsSeen[lightIndex].currentDB + "\nTime Start: " + Time.time);
                            chosenLight = LightsSeen[lightIndex];
                        }
                    }
                    else
                    {
                        lightIndex = Random.Range(0, LightsSeen.Count);
                        Debug.Log("CurrentEye: " + LightsSeen[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeen[lightIndex].x + ", " + LightsSeen[lightIndex].y + "\ncurrentDB: " + LightsSeen[lightIndex].currentDB + "\nTime Start: " + Time.time);
                        chosenLight = LightsSeen[lightIndex];
                    }
                }
                else if (LightsSeenOuter.Count > 0)
                {
                    if (Random.Range(0, 100) > 75)
                    {
                        lightIndex = Random.Range(0, LightsSeenOuter.Count);
                        Debug.Log("CurrentEye: " + LightsSeenOuter[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeenOuter[lightIndex].x + ", " + LightsSeenOuter[lightIndex].y + "\ncurrentDB: " + LightsSeenOuter[lightIndex].currentDB + "\nTime Start: " + Time.time);
                        chosenLight = LightsSeenOuter[lightIndex];
                    }
                    else
                    {
                        lightIndex = Random.Range(0, LightsSeen.Count);
                        Debug.Log("CurrentEye: " + LightsSeen[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeen[lightIndex].x + ", " + LightsSeen[lightIndex].y + "\ncurrentDB: " + LightsSeen[lightIndex].currentDB + "\nTime Start: " + Time.time);
                        chosenLight = LightsSeen[lightIndex];
                    }
                }
                else if (blinkingLightsActive.Count > 0)
                {
                    lightIndex = Random.Range(0, LightsSeen.Count);
                    Debug.Log("CurrentEye: " + LightsSeen[lightIndex].currentEye + "\nLightGridLocation x, y: " + LightsSeen[lightIndex].x + ", " + LightsSeen[lightIndex].y + "\ncurrentDB: " + LightsSeen[lightIndex].currentDB + "\nTime Start: " + Time.time);
                    chosenLight = LightsSeen[lightIndex];
                }
                else if (BlinkingBoxesActive.Count > 0)
                {
                    lightIndex = Random.Range(0, LightsSeen.Count);
                    Debug.Log("CurrentEye: " + LightsSeen[lightIndex].currentEye + "\n Quad: " + LightsSeen[lightIndex].quad + "\nCurrentDB: " + LightsSeen[lightIndex].currentDB + "\nTime Start: " + Time.time);
                    chosenLight = LightsSeen[lightIndex];
                }

                if (chosenLight != null)
                {
                    chosenLight.BlinkLight(false, !CenterPointMovementActivated);
                }

                TestStarted = true;
            }
            else if (BlinkingBoxesActive.Count > 0)
            {
                lightIndex = Random.Range(0, BlinkingBoxesActive.Count);
                Debug.Log("CurrentEye: " + LightsSeen[lightIndex].currentEye + "\n Quad: " + LightsSeen[lightIndex].quad + "\nCurrentDB: " + LightsSeen[lightIndex].currentDB + "\nTime Start: " + Time.time);
                chosenLight = BlinkingBoxesActive[lightIndex];
                TestStarted = true;
            }
            else if (blinkingLightsActive.Count == 0)
            {
                ShowResults();
            }
            else
            {
                TestStarted = true;
            }
        }

        /// <summary>
        /// StartOver:
        ///
        /// This gives the ability to start a new test after the last one has been completed
        /// </summary>
        /// <param name="testChoice"></param>
        protected virtual void StartOver(EyeTestChoice eyeTestChoice)
        {
            Debug.Log("StartOver");

            isCompleted = false;
            SetLightLocationsAndCreateLists(eyeTestChoice);
            lightsAndListsCoroutineComplete = false;
            StartCoroutine(StartTest());
        }

        /// <summary>
        /// MoveGridLocation:
        ///
        /// This version of the MoveToGrid function will move the grid to a given quad.
        /// It should only be used if no more lights are seen on screen and the grid needs to move to access more points.
        ///
        /// TODO: ADD A WAIT FOR THE PERSON TO LOOK AT CENTER POINT
        /// </summary>
        /// <param name="quad">
        /// The quadrant the grid should move to.
        /// </param>
        protected virtual void MoveGridLocation(Quadrant quad)
        {
            MovementSetToQuad = true;

            Vector3 position = new Vector3(((lCamera.transform.position.x + rCamera.transform.position.x) / 2), ((lCamera.transform.position.y + rCamera.transform.position.y) / 2), ((lCamera.transform.position.z + rCamera.transform.position.z) / 2));
            Vector3 scale = lCamera.transform.localScale;
            Quaternion rotation = lCamera.transform.rotation;
            GameObject testGO;
            Transform newTransformPos;

            testGO = new GameObject();
            //create transform that will be set from the center of the camera
            newTransformPos = testGO.transform;
            newTransformPos.position = position;
            newTransformPos.localScale = scale;
            newTransformPos.rotation = rotation;
            newTransformPos.forward = lCamera.transform.forward;
            newTransformPos.SetParent(null);

            switch (quad)
            {
                case Quadrant.UL:
                    newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - degreeOfSeparation * 2.0f), (-StartingDegreeOutward - degreeOfSeparation * 2.0f), 0));
                    break;

                case Quadrant.BL:
                    newTransformPos.Rotate(new Vector3((StartingDegreeOutward + degreeOfSeparation * 2.0f), (-StartingDegreeOutward - degreeOfSeparation * 2.0f), 0));
                    break;

                case Quadrant.UR:
                    newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - degreeOfSeparation * 2.0f), (StartingDegreeOutward + degreeOfSeparation * 2.0f), 0));
                    break;

                case Quadrant.BR:
                    newTransformPos.Rotate(new Vector3((StartingDegreeOutward + degreeOfSeparation * 2f), (StartingDegreeOutward + degreeOfSeparation * 2.0f), 0));
                    break;

                case Quadrant.Center:
                    break;

                case Quadrant.Below:
                //newTransformPos.Rotate(new Vector3((StartingDegreeOutward + degreeOfSeparation * 3f), 0, 0));
                //ray = new Ray(newTransformPos.position, newTransformPos.forward);
                //break;
                case Quadrant.Lower:
                    newTransformPos.Rotate(new Vector3((StartingDegreeOutward + degreeOfSeparation * 3f), 0, 0));
                    break;

                case Quadrant.Left:
                    newTransformPos.Rotate(new Vector3(0, (-StartingDegreeOutward - degreeOfSeparation * 2f), 0));
                    break;

                case Quadrant.Right:
                    newTransformPos.Rotate(new Vector3(0, (StartingDegreeOutward + degreeOfSeparation * 2f), 0));
                    break;

                case Quadrant.Upper:
                    newTransformPos.Rotate(new Vector3((-StartingDegreeOutward - degreeOfSeparation * 3f), 0, 0));
                    break;

                default:
                    break;
            }

            FinalCenterPos = newTransformPos.position + (newTransformPos.forward * MoveToEye.instance.Distance);
            centerPoint.transform.rotation = lCamera.transform.localRotation;

            Destroy(testGO);

            CenterPointCentered = true;
        }

        /// <summary>
        /// WhatCanBeSeen
        ///
        /// Gathers selected lights  and finds what lights can be seen on screen and adds them to a list.
        /// </summary>
        /// <param name="_activeLights"> </param>
        /// <returns></returns>
        protected virtual IEnumerator WhatCanBeSeen(List<BlinkingLight> _activeLights, bool reentry = false)
        {
            if (GridMoved == true)
            {
                LightsSeen.Clear();
                LightsSeenSeed.Clear();
                LightsSeenOuter.Clear();
                LightsSeenOutlier.Clear();
            }

            if (testChoice == EyeTestChoice.Left || testChoice == EyeTestChoice.Both)
            {
                Plane[] planes = GeometryUtility.CalculateFrustumPlanes(viewCameraL);

                foreach (var item in _activeLights)
                {
                    if (CleanList(item))
                    {
                        LightsSeen.Remove(item);
                        LightsSeenSeed.Remove(item);
                        LightsSeenOuter.Remove(item);
                        LightsSeenOutlier.Remove(item);
                        continue;
                    }
                    if (GridMoved == true)
                    {

                        if (GeometryUtility.TestPlanesAABB(planes, item.capsuleCollider.bounds) && !item.complete)
                        {
                            LightsSeen.Add(item);

                            if (item.sectionFromCenter == SectionFromCenter.Seed)
                            {
                                LightsSeenSeed.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outlier)
                            {
                                LightsSeenOutlier.Add(item);
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outer)
                            {
                                LightsSeenOuter.Add(item);
                            }
                        }
                    }
                    yield return 0f;
                }
            }

            if (testChoice == EyeTestChoice.Both || testChoice == EyeTestChoice.Right)
            {
                Plane[] planes = GeometryUtility.CalculateFrustumPlanes(viewCameraR);

                foreach (var item in _activeLights)
                {
                    if (CleanList(item))
                    {
                        LightsSeen.Remove(item);
                        LightsSeenSeed.Remove(item);
                        LightsSeenOuter.Remove(item);
                        LightsSeenOutlier.Remove(item);
                        continue;
                    }

                    if (GridMoved == true)
                    {
                        if (GeometryUtility.TestPlanesAABB(planes, item.capsuleCollider.bounds) && !item.complete)
                        {
                            if (!LightsSeen.Contains(item))
                            {
                                LightsSeen.Add(item);
                            }
                            if (item.sectionFromCenter == SectionFromCenter.Seed)
                            {
                                if (LightsSeenSeed.Contains(item))
                                {
                                    LightsSeenSeed.Add(item);
                                }
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outlier)
                            {
                                if (LightsSeenOutlier.Contains(item))
                                {
                                    LightsSeenOutlier.Add(item);
                                }
                            }
                            else if (item.sectionFromCenter == SectionFromCenter.Outer)
                            {
                                if (LightsSeenOuter.Contains(item))
                                {
                                    LightsSeenOuter.Add(item);
                                }
                            }
                        }
                    }
                    yield return 0f;
                }
            }
            GridMoved = false;
            //lightFound = true;

            if (LightsSeen.Count == 0)
            {
                Debug.Log("Light Not Found, Lights left: " + blinkingLightsActive.Count);
                SuperCleanList();
                if (_activeLights.Count > 0)
                {
                    //send back to center
                    m_TestPaused = true;
                    MoveGridLocation(QuadBeingCentered);
                    StartCoroutine(UFOScript.instance.MoveToCrosshair(QuadBeingCentered));

                    yield return new WaitUntil(() => UFOScript.instance.moved);

                    switch (_activeLights[Random.Range(0, _activeLights.Count)].quad)
                    {
                        case Quadrant.UL:
                            QuadBeingCentered = Quadrant.BR;
                            break;

                        case Quadrant.BL:
                            QuadBeingCentered = (Quadrant.UR);
                            break;

                        case Quadrant.UR:
                            QuadBeingCentered = (Quadrant.BL);
                            break;

                        case Quadrant.BR:
                            QuadBeingCentered = (Quadrant.UL);
                            break;

                        case Quadrant.Center:
                            break;

                        default:
                            break;
                    }
                    yield return new WaitForSeconds(2f);

                    StartCoroutine(UFOScript.instance.MoveOffScreen(QuadBeingCentered));

                    yield return new WaitUntil(() => UFOScript.instance.moved);

                    lightFound = false;
                    FindOppositeLightAndTestIt(_activeLights[Random.Range(0, _activeLights.Count)]);
                    GridMoved = true;
                    // wait for movement
                    yield return new WaitForSeconds(1f);
                    m_TestPaused = false;
                }
            }
            WhatCanBeSeenCompleted = true;
        }

        /// <summary>
        ///  MoveGridLocation:
        ///
        /// This version of the MoveToGrid function will move the grid to a given point.
        /// The point chosen is the point that the eye
        /// </summary>
        /// <param name="currentLight">
        /// The point that the grid needs to move to.
        /// </param>
        /// <param name="newCall"></param>
        protected virtual void MoveGridLocation(BlinkingLight currentLight = null, bool newCall = false)
        {
            Ray rayR1;
            RaycastHit raycastR1;
            MovementSetToQuad = false;

            if (moveTolight != null && moveTolight.currentEye == CurrentEye.Left)
            {
                rayR1 = new Ray(lCamera.transform.position, lCamera.transform.forward);
                Physics.Raycast(rayR1, out raycastR1, MoveToEye.instance.Distance + 1f, LAYER_LEFT);
            }
            else
            {
                rayR1 = new Ray(rCamera.transform.position, rCamera.transform.forward);
                Physics.Raycast(rayR1, out raycastR1, MoveToEye.instance.Distance + 1f, LAYER_RIGHT);
            }

            OriginalCenter = raycastR1.point;

            if (CenterPointCentered && currentLight == null && newCall)
            {
                moveTolight = currentLight;
                LightFromCenterDiff = new Vector3();
            }

            if (!CenterPointCentered && currentLight != null && moveTolight != currentLight)
            {
                CenterPointCentered = true;
                moveTolight = currentLight;
                LightFromCenterDiff = moveTolight.transform.position - (OriginalCenter);
                newCenter = LightFromCenterDiff;
            }
            else if ((CenterPointCentered && currentLight != null && moveTolight != currentLight) || (newCall && currentLight != null))
            {
                moveTolight = currentLight;
                LightFromCenterDiff = moveTolight.transform.position - (OriginalCenter);
                newCenter = LightFromCenterDiff;
            }

            if (CenterPointCentered && !isCompleted)
            {
                FinalCenterPos = OriginalCenter + LightFromCenterDiff;
            }
            else if (CenterPointCentered && isCompleted)
            {
                FinalCenterPos = OriginalCenter + LightFromCenterDiff;
                if (OriginalCenter == centerPoint.transform.position)
                {
                    CenterPointCentered = false;
                }
            }
            centerPoint.transform.rotation = Quaternion.LookRotation(centerPoint.transform.position - ((lCamera.transform.position + rCamera.transform.position) / 2f));
        }

    }
}