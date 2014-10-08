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
        
        private RegisterConnectionController(int port)
        {
            instance = this;
            this.port = port;
            hostIdentities = new Dictionary<string, HostIdentityModel>();
        }

        private void Listen()
        {/*
            var listener = new HttpListener();
            listener.Prefixes.Add("http://localhost:" + port.ToString() + "/api/RegisterConnection/");

            listener.Start();

            while (listener.IsListening)
            {
                var context = listener.GetContext();
                ThreadPool.QueueUserWorkItem((o) => IncomingConnection(context));
            }*/

            var listener = new TcpListener(IPAddress.Any, port);
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
            var bytesReceived = clientSocket.Receive(buffer);

            var text = Encoding.UTF8.GetString(buffer, 0, bytesReceived);
            var lines = text.Split(new[] { '\r', '\n' }, StringSplitOptions.None);
            int index;

            if (lines.Length < 1)
            {
                clientSocket.Dispose();
                return;
            }

            var firstLineContents = lines[0].Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
            if (firstLineContents.Length != 3 || 
                firstLineContents[0] != "POST" ||
                firstLineContents[1] != kApiRegisterPath)
            {
                clientSocket.Dispose();
                return;
            }

            int contentLength = 0;

            // Parse header
            for (index = 1; index < lines.Length && lines[index].Length > 0; index++)
            {
                var lineContents = lines[index].Split(new[] { ':', ' ' }, StringSplitOptions.RemoveEmptyEntries);

                if (lineContents.Length != 2)
                {
                    clientSocket.Dispose();
                    return;
                }

                if (string.Equals(lineContents[0], "content-type", StringComparison.OrdinalIgnoreCase))
                {
                    if (!string.Equals(lineContents[1], "application/json", StringComparison.OrdinalIgnoreCase))
                    {
                        clientSocket.Dispose();
                        return;
                    }
                }
                else if (string.Equals(lineContents[1], "content-length", StringComparison.OrdinalIgnoreCase))
                {
                    if (!int.TryParse(lineContents[1], out contentLength))
                    {
                        clientSocket.Dispose();
                        return;
                    }
                }
            }

            if (index == lines.Length || contentLength < 1)
            {
                clientSocket.Dispose();
                return;
            }

            var body = new StringBuilder(string.Concat(lines.Skip(index + 1)));

            while (body.Length < contentLength)
            {
                bytesReceived = clientSocket.Receive(buffer, Math.Min(contentLength - body.Length, buffer.Length), SocketFlags.None);
                body.Append(Encoding.UTF8.GetString(buffer, 0, bytesReceived));
            }

            HostIdentityModel model;

            try
            {
                model = JsonConvert.DeserializeObject<HostIdentityModel>(body.ToString());
            }
            catch
            {
                clientSocket.Dispose();
                return;
            }
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

            model.EndPoint = endPoint.ToString();
            hostIdentities[model.SystemUniqueId] = model;

            context.Response.StatusCode = 200;
            context.Response.KeepAlive = true;
            context.Response.Close();
        }
    }
}