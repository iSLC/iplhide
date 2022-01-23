// ------------------------------------------------------------------------------------------------
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>

// ------------------------------------------------------------------------------------------------
#include <string>
#include <vector>
#include <memory>
#include <locale>
#include <sstream>
#include <fstream>
#include <iterator>

// ------------------------------------------------------------------------------------------------
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_PNM_Image.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Display.H>

// ------------------------------------------------------------------------------------------------
namespace VcMp {

/* ------------------------------------------------------------------------------------------------
 * Main window class.
*/
class App : public virtual Fl_Double_Window
{
private:

    /* --------------------------------------------------------------------------------------------
     * The application singleton instance.
    */
    static App * s_App;

protected:

    // --------------------------------------------------------------------------------------------
    Fl_File_Chooser    *m_InputFC; // Input file chooser
    Fl_Input           *m_InputPath; // Input file path
    Fl_Button          *m_InputShow; // Show the input file chooser
    Fl_File_Icon       *m_InputIcon; // Icon for the input show button

    // --------------------------------------------------------------------------------------------
    Fl_Text_Display    *m_OutputBox; // Buffer display
    Fl_Text_Buffer     *m_OutputBuffer; // Output buffer

    // --------------------------------------------------------------------------------------------
    Fl_Button          *m_BuildXML; // Generate XML data
    Fl_Button          *m_BuildNUT; // Generate NUT data with unmodified coordinates
    Fl_Button          *m_BuildRAW; // Generate RAW data with adjusted coordinates

    // --------------------------------------------------------------------------------------------
    Fl_Input           *m_FuncName; // Function name input

private:

    /* --------------------------------------------------------------------------------------------
     * An instance in the IPL file.
    */
    struct Instance
    {
        // ----------------------------------------------------------------------------------------
        int   mID; // Model IP
        double mX, mY, mZ; // Model position

        /* ----------------------------------------------------------------------------------------
         * Base constructor.
        */
        Instance(int id, double x,  double y,  double z)
            : mID(id), mX(x), mY(y), mZ(z)
        {
            /* ... */
        }

        /* ----------------------------------------------------------------------------------------
         * Copy constructor.
        */
        Instance(const Instance & o) = default;

        /* ----------------------------------------------------------------------------------------
         * Move constructor.
        */
        Instance(Instance && o) = default;

        /* ----------------------------------------------------------------------------------------
         * Destructor.
        */
        ~Instance() = default;

        /* ----------------------------------------------------------------------------------------
         * Copy assignment operator.
        */
        Instance & operator = (const Instance & o) = default;

        /* ----------------------------------------------------------------------------------------
         * Move assignment operator.
        */
        Instance & operator = (Instance && o) = default;
    };

    // --------------------------------------------------------------------------------------------
    typedef std::vector< Instance > Instances; // Instance list

    /* --------------------------------------------------------------------------------------------
     * Encapsulates character classification for tokenization of the IPL data.
    */
    struct Tokens: std::ctype< char >
    {
        /* ----------------------------------------------------------------------------------------
         * Default constructor.
        */
        Tokens()
            : std::ctype< char >(get_table())
        {
            /* ... */
        }

        /* ----------------------------------------------------------------------------------------
         * Generate and returns the characters to be used by std::basic_istream.
        */
        static std::ctype_base::mask const * get_table()
        {
            typedef std::ctype< char > cctype;
            static const cctype::mask * const_rc = cctype::classic_table();

            static cctype::mask rc[cctype::table_size];
            std::memcpy(rc, const_rc, cctype::table_size * sizeof(cctype::mask));

            rc[','] = std::ctype_base::space;
            rc[' '] = std::ctype_base::space;

            return &rc[0];
        }
    };

    /* --------------------------------------------------------------------------------------------
     * Default constructor.
    */
    App()
        : Fl_Double_Window(WindowWidth, WindowHeight, "IPL Hide")
        , m_InputFC(nullptr)
        , m_InputPath(nullptr)
        , m_InputShow(nullptr)
        , m_InputIcon(nullptr)
        , m_OutputBox(nullptr)
        , m_OutputBuffer(nullptr)
        , m_BuildXML(nullptr)
        , m_BuildNUT(nullptr)
        , m_BuildRAW(nullptr)
        , m_FuncName(nullptr)
    {
        this->begin();
        // ----------------------------------------------------------------------------------------
        m_InputFC = new Fl_File_Chooser(".", "Vice City IPL Files (*.ipl)",
                                            Fl_File_Chooser::SINGLE, "Select the Input file...");
        // ----------------------------------------------------------------------------------------
        m_InputPath = new Fl_Input(48, 8, 548, 24, "Input:");
        m_InputPath->value("");
        // ----------------------------------------------------------------------------------------
        m_InputShow = new Fl_Button(600, 8, 24, 24);
        m_InputShow->labelcolor(FL_YELLOW);
        m_InputShow->callback(&App::InputShowCallback);
        // ----------------------------------------------------------------------------------------
        m_InputIcon = Fl_File_Icon::find(".", Fl_File_Icon::DIRECTORY);
        m_InputIcon->label(m_InputShow);
        // ----------------------------------------------------------------------------------------
        m_OutputBox = new Fl_Text_Display(8, 86, 622, 384);
        // ----------------------------------------------------------------------------------------
        m_OutputBuffer = new Fl_Text_Buffer(1024 * 1024 * 4);
        m_OutputBox->buffer(m_OutputBuffer);
        // ----------------------------------------------------------------------------------------
        m_BuildXML = new Fl_Button(8, 48, 48, 24, "XML");
        m_BuildXML->callback(&App::BuildXMLCallback);
        // ----------------------------------------------------------------------------------------
        m_BuildNUT = new Fl_Button(64, 48, 48, 24, "NUT");
        m_BuildNUT->callback(&App::BuildNUTCallback);
        // ----------------------------------------------------------------------------------------
        m_BuildRAW = new Fl_Button(120, 48, 48, 24, "RAW");
        m_BuildRAW->callback(&App::BuildRAWCallback);
        // ----------------------------------------------------------------------------------------
        m_FuncName = new Fl_Input(250, 48, 380, 24, "Function:");
        m_FuncName->value("HideMapObject");
        // ----------------------------------------------------------------------------------------
        this->end();
    }

    /* --------------------------------------------------------------------------------------------
     * Copy constructor. (disabled)
    */
    App(const App &) = delete;

    /* --------------------------------------------------------------------------------------------
     * Move constructor. (disabled)
    */
    App(App &&) = delete;

    /* --------------------------------------------------------------------------------------------
     * Copy assignment operator. (disabled)
    */
    App & operator = (const App &) = delete;

    /* --------------------------------------------------------------------------------------------
     * Move assignment operator. (disabled)
    */
    App & operator = (App &&) = delete;

public:

    // --------------------------------------------------------------------------------------------
    static const int WindowWidth; // Default window width
    static const int WindowHeight; // Default window height

    /* --------------------------------------------------------------------------------------------
     * Destructor.
    */
    virtual ~App()
    {
        /* ... */
    }

    /* --------------------------------------------------------------------------------------------
     * Retrieve the application instance.
    */
    static App * Get()
    {
        if (!s_App)
        {
            s_App = new App();
        }

        return s_App;
    }

    /* --------------------------------------------------------------------------------------------
     * Exit the application.
    */
    void Exit()
    {
        std::exit(EXIT_SUCCESS);
    }

    /* --------------------------------------------------------------------------------------------
     * Generate XML output from the selected IPL file.
    */
    void BuildXML()
    {
        // Allocate a list of instances
        Instances inst_list;
        // Populate the list with elements from the selected IPL file
        if (!Extract(inst_list))
        {
            return; // Nothing to process
        }
        // Clear any previous data from the buffer
        m_OutputBuffer->text("");
        // Buffer for the generated rule
        char buffer[512] = {0};
        // Format result and line count
        int ret = 0, lnum  = 0;
        // Process all instances in the list
        for (const auto & inst : inst_list)
        {
            // Advance the line count
            ++lnum;
            // Attempt to generate the map rule
            ret = std::snprintf(buffer, sizeof(buffer),
                "<rule model=\"%d\">\n\t<position x=\"%f\" y=\"%f\" z=\"%f\" />\n</rule>\n",
                inst.mID, inst.mX, inst.mY, inst.mZ);
            // Did an error occurred?
            if (ret < 0)
            {
                fl_alert("A format error occurred at line: %d", lnum);
                // We're done here
                return;
            }
            // Did we have enough buffer?
            else if (static_cast< unsigned >(ret) > sizeof(buffer))
            {
                fl_alert("A buffer overflow occurred at line: %d", lnum);
                // We're done here
                return;
            }
            // Add the resulted rule string to the buffer
            m_OutputBuffer->append(buffer);
        }
    }

    /* --------------------------------------------------------------------------------------------
     * Generate NUT output from the selected IPL file.
    */
    void BuildNUT()
    {
        // Allocate a list of instances
        Instances inst_list;
        // Populate the list with elements from the selected IPL file
        if (!Extract(inst_list))
        {
            return; // Nothing to process
        }
        // Clear any previous data from the buffer
        m_OutputBuffer->text("");
        // Buffer for the generated line of code
        char buffer[512] = {0};
        // Format result and line count
        int ret = 0, lnum  = 0;
        // Process all instances in the list
        for (const auto & inst : inst_list)
        {
            // Advance the line count
            ++lnum;
            // Attempt to generate the function call code
            ret = std::snprintf(buffer, sizeof(buffer), "%s(%d, %f, %f, %f);\n",
                                m_FuncName->value(), inst.mID, inst.mX, inst.mY, inst.mZ);
            // Did an error occurred?
            if (ret < 0)
            {
                fl_alert("A format error occurred at line: %d", lnum);
                // We're done here
                return;
            }
            // Did we have enough buffer?
            else if (static_cast< unsigned >(ret) > sizeof(buffer))
            {
                fl_alert("A buffer overflow occurred at line: %d", lnum);
                // We're done here
                return;
            }
            // Add the resulted code string to the buffer
            m_OutputBuffer->append(buffer);
        }
    }

    /* --------------------------------------------------------------------------------------------
     * Generate RAW NUT output from the selected IPL file.
    */
    void BuildRAW()
    {
        // Allocate a list of instances
        Instances inst_list;
        // Populate the list with elements from the selected IPL file
        if (!Extract(inst_list))
        {
            return; // Nothing to process
        }
        // Clear any previous data from the buffer
        m_OutputBuffer->text("");
        // Buffer for the generated line of code
        char buffer[512] = {0};
        // Format result and line count
        int ret = 0, lnum  = 0;
        // Computed coordinates
        int x = 0, y = 0, z = 0;
        // Process all instances in the list
        for (const auto & inst : inst_list)
        {
            // Advance the line count
            ++lnum;
            // Compute coordinates
            x = static_cast< int >(std::floor(inst.mX * 10.0) + 0.5);
            y = static_cast< int >(std::floor(inst.mY * 10.0) + 0.5);
            z = static_cast< int >(std::floor(inst.mZ * 10.0) + 0.5);
            // Attempt to generate the function call code
            ret = std::snprintf(buffer, sizeof(buffer), "%s(%d, %d, %d, %d);\n",
                                m_FuncName->value(), inst.mID, x, y, z);
            // Did an error occurred?
            if (ret < 0)
            {
                fl_alert("A format error occurred at line: %d", lnum);
                // We're done here
                return;
            }
            // Did we have enough buffer?
            else if (static_cast< unsigned >(ret) > sizeof(buffer))
            {
                fl_alert("A buffer overflow occurred at line: %d", lnum);
                // We're done here
                return;
            }
            // Add the resulted code string to the buffer
            m_OutputBuffer->append(buffer);
        }
    }

protected:

    // --------------------------------------------------------------------------------------------
    typedef std::vector< std::string > Strings;

    /* --------------------------------------------------------------------------------------------
     * Explode the specified instance definition into individual values.
    */
    static Strings Explode(const std::string & str)
    {
        // Obtain a stream fro the specified string
        std::stringstream ss(str);
        // Associate our custom locale with the stream
        ss.imbue(std::locale(std::locale(), new Tokens()));
        // Construct and return a vector with the extracted tokens
        return Strings(std::istream_iterator< std::string >(ss),
                        std::istream_iterator< std::string >());
    }

    /* --------------------------------------------------------------------------------------------
     * Extract the values from the specified IPL file.
    */
    bool Extract(Instances & inst_list)
    {
        // Characters that represent a space
        static const char schars[] = {' ', '\t', '\n', '\v', '\f', '\r', '\0'};
        // Retrieve the path from the input widget
        const char * iplpath = m_InputPath->value();
        // See if a path was selected and whether its valid
        if (!iplpath || *iplpath == '\0')
        {
            fl_alert("No IPL file selected");
            // We're done here
            return false;
        }
        // Attempt to open the specified file
        std::ifstream iplfile(iplpath);
        // See if the file could be opened
        if (!iplfile.is_open())
        {
            fl_alert("Unable to open the selected file");
            // We're done here
            return false;
        }
        // String where the current line is retrieved
        std::string line;
        // Whether the we reached the instances section
        bool in_inst = false;
        // Line count
        int lnum = 0;
        // Process the file line by line
        while (std::getline(iplfile, line))
        {
            // Advance the line count
            ++lnum;
            // Is the first character a space?
            if (std::isspace(line.front()) != 0)
            {
                // Find the first non space character
                std::string::size_type pos = line.find_first_not_of(schars);
                // Have we found anything?
                if (pos == std::string::npos)
                {
                    continue; // Skip this line!
                }
                // Slice out the space portion
                line = line.substr(pos);
            }
            // Is this line a comment?
            if (line.front() == '#')
            {
                continue; // Nothing to process here!
            }
            // Are we supposed to stop processing instances?
            if (in_inst && line.compare("end") == 0)
            {
                break; // Nothing left to parse!
            }
            // Are we supposed to process an instance?
            else if (in_inst)
            {
                // The list of individual values from the line
                Strings args;
                // Extract the individual values from the string
                try
                {
                    args = Explode(line);
                }
                catch (...)
                {
                    fl_alert("Error while tokenizing line: %d", lnum);
                    // Skip this line...
                    continue;
                }
                // Do we even have enough values?
                if (args.size() < 6)
                {
                    fl_alert("Wrong number of tokens at line: %d", lnum);
                    // This instance is incomplete!
                    continue;
                }
                // Attempt to add this instance to the list
                try
                {
                    inst_list.emplace_back(
                        std::stoi(args[0], nullptr, 10),
                        std::stod(args[3], nullptr),
                        std::stod(args[4], nullptr),
                        std::stod(args[5], nullptr)
                    );
                }
                catch (...)
                {
                    fl_alert("Unable to extract values at line: %d", lnum);
                }
            }
            else if (line.compare("inst") == 0)
            {
                // We entered the instance section
                in_inst = true;
                // Skip this line
                continue;
            }
        }
        // Return whether we have anything to give to the caller
        return (in_inst && !inst_list.empty());
    }

    /* --------------------------------------------------------------------------------------------
     * Show the input file selection dialog.
    */
    static void InputShowCallback(Fl_Widget * /*w*/, void * /*p*/)
    {
        // Show the file selection dialog
        s_App->m_InputFC->show();
        // Wait until the dialog is closed
        while (s_App->m_InputFC->visible())
        {
            Fl::wait();
        }
        // Have we selected anything?
        if (s_App->m_InputFC->count() >= 1)
        {
            // Store the first path
            s_App->m_InputPath->value(s_App->m_InputFC->value(1));
        }
    }

    /* --------------------------------------------------------------------------------------------
     * Generate XML output from the selected IPL file.
    */
    static void BuildXMLCallback(Fl_Widget * w, void * p)
    {
        s_App->BuildXML();
    }

    /* --------------------------------------------------------------------------------------------
     * Generate NUT output from the selected IPL file.
    */
    static void BuildNUTCallback(Fl_Widget * w, void * p)
    {
        s_App->BuildNUT();
    }

    /* --------------------------------------------------------------------------------------------
     * Generate RAW NUT output from the selected IPL file.
    */
    static void BuildRAWCallback(Fl_Widget * w, void * p)
    {
        s_App->BuildRAW();
    }
};

// ------------------------------------------------------------------------------------------------
const int App::WindowWidth  = 640;
const int App::WindowHeight = 480;

// ------------------------------------------------------------------------------------------------
App * App::s_App = nullptr;

} // Namespace:: VcMp

// ------------------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    using namespace VcMp;
    Fl::scheme(nullptr);
    Fl_File_Icon::load_system_icons();
    App * w = App::Get();
    w->show(argc, argv);
    return Fl::run();
}
