using BackendServer.Models;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Threading;
using System.Web;

namespace BackendServer.Controllers
{
    public class RegisterConnectionController
    {
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
        {
            var listener = new HttpListener();
            listener.Prefixes.Add("http://localhost:" + port.ToString() + "/api/RegisterConnection/");

            listener.Start();

            while (listener.IsListening)
            {
                var context = listener.GetContext();
                ThreadPool.QueueUserWorkItem((o) => IncomingConnection(context));
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