using BackendServer.Models;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Web;
using System.Web.Mvc;

namespace BackendServer.Controllers
{
    public class HomeController : Controller
    {
        public ActionResult Index()
        {
            return View(new FileHostModel());
        }

        [HttpPost]
        [AllowAnonymous]
        public ActionResult Index(FileHostModel model)
        {
            if (!ModelState.IsValid)
            {
                return View(model);
            }

            var host = RegisterConnectionController.Instance[model.FileHostCode];

            if (host != null && SendClientIP(host, this.Request.UserHostAddress))
            {
                return Redirect(host.HostSocket.RemoteEndPoint.ToString());
            }
            
            ModelState.AddModelError("errorSummary", "The specified file host doesn't exist.");
            return View(model);
        }

        private bool SendClientIP(HostIdentityModel host, string ip)
        {
            if (!host.HostSocket.Connected)
            {
                RegisterConnectionController.Instance.DropConnection(host.SystemUniqueId);
                return false;
            }

            var requestBuilder = new StringBuilder();

            requestBuilder.Append("POST api/clientip/ HTTP/1.1\n");
            requestBuilder.Append("content-type: applicaton/json\n");

            var clientIdentityModel = new ClientIdentityModel() { IpAddress = ip };
            var content = Json(clientIdentityModel).ToString();

            requestBuilder.Append("content-length: ");
            requestBuilder.Append(content.Length.ToString());
            requestBuilder.Append("\n\n");
            requestBuilder.Append(content);

            string response;

            try
            {
                host.HostSocket.Send(Encoding.UTF8.GetBytes(content));

                var buffer = new byte[256];
                var bytesReceived = host.HostSocket.Receive(buffer);
                response = Encoding.UTF8.GetString(buffer, 0, bytesReceived);
            }
            catch
            {
                RegisterConnectionController.Instance.DropConnection(host.SystemUniqueId);
                return false;
            }

            return response.StartsWith("HTTP/1.1 200 OK");
        }

    }    
}