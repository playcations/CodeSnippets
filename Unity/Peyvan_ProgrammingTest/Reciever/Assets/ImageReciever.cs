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
    /// messages to be sent with packets of the texture;
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
    /// Recieves image once connected
    /// </summary>
    public class ImageReciever : MonoBehaviour
    {
        private Texture2D texture;
        private Dictionary<uint, byte[]> savedData;
        private NetworkClient myClient;

        public InputField IPAddress, Port;
        public GameObject planeObj;

        private byte[] byteImage;
        // Start is called before the first frame update
        void Start()
        {
            IPAddress.text = IPManager.GetIP();

            savedData = new Dictionary<uint, byte[]>();
        }

        /// <summary>
        /// Class called from button to start server and then wait for texture info
        /// </summary>
        public void RecieveImage()
        {
            SetupServer(Port.text);
        }

        /// <summary>
        /// Start listening for clients wanting to connect.
        /// </summary>
        /// <param name="port"></param>
        public void SetupServer(string port)
        {
            try
            {
                NetworkServer.Listen(int.Parse(port));
                Debug.Log("Started Listening");
            }
            catch (System.Exception ex)
            {
                Debug.Log("ERROR: " + ex.ToString());
            }

            NetworkServer.RegisterHandler(TextureMsgType.Texture, OnMessage);
            NetworkServer.RegisterHandler(TextureMsgType.Complete, OnComplete);
        }

        /// <summary>
        /// Once messased, starts loading image
        /// </summary>
        /// <param name="netMsg"></param>
        public void OnMessage(NetworkMessage netMsg)
        {
            TextureMessage msg = netMsg.ReadMessage<TextureMessage>();
            savedData[msg.index] = msg.texture;
            Debug.Log("Packet Recieved, Asking for more");
            TextureMessage empty = new TextureMessage();
            NetworkServer.SendToAll(TextureMsgType.AnothaOne, empty);
        }

        /// <summary>
        /// once all parts are sent, builds texture and shows it off on plane
        /// </summary>
        /// <param name="netMsg"></param>
        public void OnComplete(NetworkMessage netMsg)
        {
            TextureMessage msg = netMsg.ReadMessage<TextureMessage>();
            Texture2D t = new Texture2D(2048, 2048, TextureFormat.RGB24, false);
            byte[] finalBytes = new byte[msg.index];
            foreach (var chunk in savedData)
            {
                System.Array.Copy(chunk.Value, 0, finalBytes, chunk.Key, chunk.Value.Length);
            }
            Debug.Log("OnCompleteReached");
            new Thread(() =>
            {
                UnityMainThreadDispatcher.Instance().Enqueue(() =>
               {
                   t.LoadRawTextureData(finalBytes);
                   t.Apply();
                   planeObj.GetComponent<MeshRenderer>().material.mainTexture = t;
               });
            }).Start();
        }
    }

}