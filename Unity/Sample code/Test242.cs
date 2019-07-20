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
    public class Test242 : TestBaseClass
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
    }
}