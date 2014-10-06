using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Web;

namespace BackendServer.Models
{
    public class FileHostModel
    {
        [Required]
        [Display(Name = "File host code")]
        public string FileHostCode { get; set; }
    }
}