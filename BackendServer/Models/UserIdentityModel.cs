using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Web;

namespace BackendServer.Models
{
    public class UserIdentityModel
    {
        [Required]
        public string SystemUniqueId { get; set; }
    }
}