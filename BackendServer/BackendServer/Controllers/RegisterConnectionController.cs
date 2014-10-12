using BackendServer.Models;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Web;

namespace BackendServer.Controllers
{
    public class RegisterConnectionController
    {
        const string kApiRegisterPath = "/api/RegisterConnection/";

        public static void StartListening(int port)
        {
            if (instance != null)
            {
                throw new InvalidOperationException("Already listening!");
            }

            new Thread(() =>
            {
                new RegisterConnectionController(port).Listen();
            }).Start();
        }

        private static RegisterConnectionController instance;
        private int port;
        private Dictionary<string, HostIdentityModel> hostIdentities;

        public static RegisterConnectionController Instance { get { return instance; } }
        
        public HostIdentityModel this[string hostId]
        {
            get
            {
                HostIdentityModel model;

                lock (hostIdentities)
                {
                    if (hostIdentities.TryGetValue(hostId, out model))
                    {
                        return model;
                    }
                }

                return null;
            }
        }

        public void DropConnection(string hostId)
        {
            lock (hostIdentities)
            {
                if (!hostIdentities.ContainsKey(hostId))
                {
                    return;
                }
                
                hostIdentities[hostId].HostSocket.Dispose();
                hostIdentities.Remove(hostId);
            }
        }

        private RegisterConnectionController(int port)
        {
            instance = this;
            this.port = port;
            hostIdentities = new Dictionary<string, HostIdentityModel>();
        }

        private void Listen()
        {
            var listener = TcpListener.Create(port);
            listener.Start();

            for (;;)
            {
                var clientSocket = listener.AcceptSocket();
                ThreadPool.QueueUserWorkItem((o) => IncomingConnection(clientSocket));
            }
        }

        private void IncomingConnection(Socket clientSocket)
        {
            byte[] buffer = new byte[2560];
            int bytesReceived = 0;

            try
            {
                bytesReceived = clientSocket.Receive(buffer);
            }
            catch
            {
                RefuseConnection(clientSocket);
                return;
            }

            var text = Encoding.UTF8.GetString(buffer, 0, bytesReceived);
            var lines = text.Split(new[] { '\r', '\n' }, StringSplitOptions.None);
            int index;

            if (lines.Length < 1)
            {
                RefuseConnection(clientSocket);
                return;
            }

            var firstLineContents = lines[0].Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
            if (firstLineContents.Length != 3 || 
                firstLineContents[0] != "POST" ||
                firstLineContents[1] != kApiRegisterPath)
            {
                RefuseConnection(clientSocket);
                return;
            }

            int contentLength = 0;

            // Parse header
            for (index = 1; index < lines.Length && lines[index].Length > 0; index++)
            {
                var lineContents = lines[index].Split(new[] { ':', ' ' }, StringSplitOptions.RemoveEmptyEntries);

                if (lineContents.Length != 2)
                {
                    RefuseConnection(clientSocket);
                    return;
                }

                if (string.Equals(lineContents[0], "content-type", StringComparison.OrdinalIgnoreCase))
                {
                    if (!string.Equals(lineContents[1], "application/json", StringComparison.OrdinalIgnoreCase))
                    {
                        RefuseConnection(clientSocket);
                        return;
                    }
                }
                else if (string.Equals(lineContents[0], "content-length", StringComparison.OrdinalIgnoreCase))
                {
                    if (!int.TryParse(lineContents[1], out contentLength))
                    {
                        RefuseConnection(clientSocket);
                        return;
                    }
                }
            }

            if (index == lines.Length || contentLength < 1)
            {
                RefuseConnection(clientSocket);
                return;
            }

            var body = new StringBuilder(string.Concat(lines.Skip(index + 1)));

            while (body.Length < contentLength)
            {
                try
                {
                    bytesReceived = clientSocket.Receive(buffer, Math.Min(contentLength - body.Length, buffer.Length), SocketFlags.None);
                }
                catch
                {
                    RefuseConnection(clientSocket);
                    return;
                }
                body.Append(Encoding.UTF8.GetString(buffer, 0, bytesReceived));
            }

            HostIdentityModel model;

            try
            {
                model = JsonConvert.DeserializeObject<HostIdentityModel>(body.ToString());
            }
            catch
            {
                RefuseConnection(clientSocket);
                return;
            }

            model.HostSocket = clientSocket;

            lock (hostIdentities)
            {
                hostIdentities[model.SystemUniqueId] = model;
            }

            if (!SendResponse(clientSocket, true))
            {
                DropConnection(model.SystemUniqueId);
            }
        }

        private bool SendResponse(Socket socket, bool success)
        {
            string responseString;

            if (success)
            {
                responseString = "HTTP/1.1 200 OK\n";
            }
            else
            {
                responseString = "HTTP/1.1 400 Bad Request\n";
            }

            var bytes = Encoding.UTF8.GetBytes(responseString);

            try
            {
                socket.Send(bytes);
            }
            catch
            {
                return false;
            }

            return true;
        }

        private void RefuseConnection(Socket socket)
        {
            SendResponse(socket, false);
            socket.Dispose();
        }

        private void IncomingConnection(HttpListenerContext context)
        {
            var request = context.Request;

            if (request.HttpMethod != "POST")
            {
                context.Response.StatusCode = 400;
                context.Response.Close();
                return;
            }

            var endPoint = request.RemoteEndPoint;
            var postBody = new StreamReader(request.InputStream).ReadToEnd();

            HostIdentityModel model;

            try
            {
                model = JsonConvert.DeserializeObject<HostIdentityModel>(postBody);
            }
            catch
            {
                context.Response.StatusCode = 400;
                context.Response.Close();
                return;
            }

       //     model.EndPoint = endPoint.ToString();
       //     hostIdentities[model.SystemUniqueId] = model;

            context.Response.StatusCode = 200;
            context.Response.KeepAlive = true;
            context.Response.Close();
        }
    }
}