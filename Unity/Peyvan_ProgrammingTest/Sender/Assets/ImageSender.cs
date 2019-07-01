using System.Collections;
using System.Collections.Generic;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Threading;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

namespace Networting
{
    /// <summary>
    /// Data chunk class for sending info over network
    /// </summary>
    [System.Serializable]
    public class TextureMessage : MessageBase
    {
        public uint index = 0;
        public byte[] texture;
    }

    /// <summary>
    /// Message handler
    /// </summary>
    public class TextureMsgType
    {
        public static short Texture = MsgType.Highest + 1;
        public static short Complete = MsgType.Highest + 2;
        public static short AnothaOne = MsgType.Highest + 3;
    }

    /// <summary>
    /// Get the local IP address for a starting point
    /// </summary>
    public class IPManager
    {
        public static string GetIP()
        {
            string output = "";

            foreach (NetworkInterface item in NetworkInterface.GetAllNetworkInterfaces())
            {
#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
                NetworkInterfaceType _type1 = NetworkInterfaceType.Wireless80211;
                NetworkInterfaceType _type2 = NetworkInterfaceType.Ethernet;

                if ((item.NetworkInterfaceType == _type1 || item.NetworkInterfaceType == _type2)
                    && item.OperationalStatus == OperationalStatus.Up)
#endif
                {
                    foreach (UnicastIPAddressInformation ip in item.GetIPProperties().UnicastAddresses)
                    {
                        if (ip.Address.AddressFamily == AddressFamily.InterNetwork)
                        {
                            output = ip.Address.ToString();
                        }
                    }
                }
            }
            return output;
        }
    }

    /// <summary>
    /// Class that sends texture over network
    /// </summary>
    public class ImageSender : MonoBehaviour
    {
        private Texture2D texture;
        private NetworkClient myClient;

        public InputField IPAddress, Port;
        public GameObject planeObj;

        private byte[] byteImage;

        private uint currentIndex = 0;
        private uint miniindex = 0;
        private uint bufferSize = 63000;

        // Start is called before the first frame update
        void Start()
        {
            IPAddress.text = IPManager.GetIP();
        }


        /// <summary>
        /// Class called from button to start sending texture
        /// </summary>
        public void SendImage()
        {
            new Thread(() =>
           {
               UnityMainThreadDispatcher.Instance().Enqueue(() =>
               {
                   texture = planeObj.GetComponent<MeshRenderer>().material.mainTexture as Texture2D;
                   byteImage = texture.GetRawTextureData();
                   SetupClient(IPAddress.text, Port.text);
               });
           }).Start();
        }

        /// <summary>
        ///  Create a client and connect to the server port
        /// </summary>
        public void SetupClient(string ip, string port)
        {
            myClient = new NetworkClient();
            myClient.RegisterHandler(MsgType.Connect, OnConnected);
            myClient.RegisterHandler(TextureMsgType.AnothaOne, AnothaOne);
            Debug.Log("Connecting");
            try
            {
                myClient.Connect(ip, int.Parse(port));
            }
            catch (System.Exception ex)
            {
                Debug.Log("ERROR: " + ex.ToString());
            }
        }

        /// <summary>
        /// Once connected, send the image
        /// </summary>
        public void OnConnected(NetworkMessage netMsg)
        {
            Debug.Log("Connected to server");
            StartCoroutine(SendBytes());
        }

        /// <summary>
        /// sends chunk of bytes for texture
        /// </summary>
        /// <returns></returns>
        public IEnumerator SendBytes()
        {
            yield return null;
            if (currentIndex < byteImage.Length - 1)
            {
                yield return new WaitForSeconds(.5f);

                int remaining = byteImage.Length - (int)currentIndex;
                if (remaining < bufferSize)
                    bufferSize = (uint)remaining;

                TextureMessage mb = new TextureMessage();
                mb.texture = new byte[bufferSize];

                System.Array.Copy(byteImage, currentIndex, mb.texture, 0, bufferSize);

                mb.index = currentIndex;
                myClient.Send(TextureMsgType.Texture, mb);

                currentIndex += bufferSize;
                miniindex++;
            }
            else
            {

                TextureMessage complete = new TextureMessage();
                complete.index = (uint)byteImage.Length;
                myClient.Send(TextureMsgType.Complete, complete);
            }
            yield return null;
        }

        /// <summary>
        /// Starts request for next chunk of data
        /// </summary>
        /// <param name="netMsg"></param>
        public void AnothaOne(NetworkMessage netMsg)
        {
            StartCoroutine(SendBytes());
        }

    }

}