using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/// <summary>
/// Test 24-2
///
/// The Shortest of the 3 main tests.
/// Total Lights: 54
/// Degrees of separation per light: 3
///
/// </summary>
namespace Testing
{
    public class Test242Static : TestBaseClass
    {
        // Use this for initialization
        private void Start()
        {
            centerPoint.GetComponent<MeshRenderer>().enabled = true;
            GlobalStartingProcedures();
            quadLightCount = 13;
            gridSize = 3;
            StartingDegreeOutward = 3f;
            degreeOfSeparation = 6f;
        }

        // Update is called once per frame
        private void Update()
        {
            CenterGridOnUpdate();
        }

        /// <summary>
        /// StartTest:
        ///
        /// Starts Chosen Test
        /// </summary>
        public override IEnumerator StartTest()
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
                        if (arrowPointer.IsStaringAtCross())
                        {
                            Debug.Log("RunNextTest(Boxes)");
                            StartCoroutine(RunNextTest(BlinkingBoxesActive));

                            yield return new WaitForSeconds(1.5f);
                            yield return new WaitUntil(() => TestStarted);
                            Debug.Log("Time End: " + Time.time);

                            if (LightsSeen.Count > 0)
                            {
                                lightFound = false;
                                TestStarted = false;
                            }
                            else
                            {
                                TestStarted = false;
                            }
                            CleanList(chosenLight, true);
                        }
                        else
                        {
                            yield return new WaitForEndOfFrame();
                        }
                    }
                }

                ///Build Lights Here
                ///TestRemainingQuadrants
                BuildQuadsFromBoxes();
            }

            while (blinkingLightsSeed.Count > 2)
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
                            Debug.Log("RunNextTest(blinkingLightsActive with seeds)");
                            StartCoroutine(RunNextTest(blinkingLightsActive));

                            yield return new WaitForSeconds(1f);
                            if (chosenLight != null)
                            {
                                yield return new WaitUntil(() => !chosenLight.isActive);
                            }

                            Debug.Log("Time End: " + Time.time);

                            if (LightsSeen.Count > 0)
                            {
                                lightFound = false;
                                TestStarted = false;
                                CleanList(chosenLight);
                            }
                            else
                            {
                                TestStarted = false;
                            }
                        }
                    }
                    else
                    {
                        yield return new WaitForEndOfFrame();
                    }
                }
            }

            //Set the rest of the lights up with the correct DB
            // SetDBOnRestOfLights();
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
                    //TODO add Pause notification
                    currentlyBlinking = false;
                    yield return new WaitForSeconds(0.5f);
                }
                else
                {
                    if (arrowPointer.IsStaringAtCross())
                    {
                        int _falseTestCheck = 0;
                        _falseTestCheck = RunAFalseTestCheck();

                        if (_falseTestCheck == 1)
                        {
                            Debug.Log("Running False Negative" + "\nTime Start: " + Time.time);
                            RunFalseNegative();
                            yield return new WaitUntil(() => falseNegative.isActive == false);
                            Debug.Log("Time End: " + Time.time);
                        }
                        else if (_falseTestCheck == 2)
                        {
                            Debug.Log("Running False Positive" + "\nTime Start: " + Time.time);
                            RunFalsePositive();
                            yield return new WaitUntil(() => falsePositive.isActive == false);
                            Debug.Log("Time End: " + Time.time);
                        }
                        else
                        {
                            Debug.Log("RunNextTest");

                            StartCoroutine(RunNextTest(blinkingLightsActive));

                            yield return new WaitForSeconds(1.5f);
                            yield return new WaitUntil(() => TestStarted);
                            yield return new WaitUntil(() => !m_TestPaused);

                            if (LightsSeen.Count > 0)
                            {
                                lightFound = false;
                                TestStarted = false;
                            }
                            else
                            {
                                TestStarted = false;
                            }
                            CleanList(chosenLight);
                        }
                    }
                    else
                    {
                        yield return new WaitForEndOfFrame();
                    }
                }
            }
            //run complete info here
            ShowResults();
        }

        /// <summary>
        /// WhatCanBeSeen
        ///
        /// Gathers selected lights  and finds what lights can be seen on screen and adds them to a list.
        /// </summary>
        /// <param name="_activeLights"> </param>
        /// <returns></returns>
        protected override IEnumerator WhatCanBeSeen(List<BlinkingLight> _activeLights, bool reentry = false)
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
                    GridMoved = true;
                    // wait for movement
                    yield return new WaitForSeconds(1f);
                    m_TestPaused = false;
                }
            }
            WhatCanBeSeenCompleted = true;
        }
    }
}